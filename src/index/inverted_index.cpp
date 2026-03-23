/*
 * inverted_index.cpp  —  LMDB-backed persistent inverted index (Inode-Only)
 *
 * Databases inside the single MDB_env:
 *   db_files    : ino(u64)     -> mtime(u64)       (unique, INTEGERKEY)
 *   db_word2id  : word(str)    -> word_id(u32)     (unique)
 *   db_id2word  : word_id(u32) -> word(str)        (unique, INTEGERKEY)
 *   db_inverted : word_id(u32) -> ino(u64)         (DUPSORT | INTEGERKEY |
 * DUPFIXED) db_forward  : ino(u64)     -> word_id(u32)     (DUPSORT |
 * INTEGERKEY | DUPFIXED)
 *
 * A special sentinel key "__meta_last_sync__" in db_files stores the
 * last global sync timestamp (uint64_t) for differential crawl support.
 * The key is constructed as a 0-inode or special bit-pattern to avoid
 * collision.
 */

#include "myheader.h"
#include <cstdint>
#include <filesystem>
#include <lmdb.h>

static void lmdb_check(int rc, const char *op) {
  if (rc != MDB_SUCCESS && rc != MDB_NOTFOUND) {
    logMessage(std::string("LMDB error in ") + op + ": " + mdb_strerror(rc));
  }
}

void InvertedIndex::getStat(const std::string &path, uint64_t &ino,
                            uint64_t &dev) {
  struct stat st;
  if (::stat(path.c_str(), &st) == 0) {
    ino = st.st_ino;
    dev = st.st_dev;
  } else {
    ino = 0;
    dev = 0;
  }
}

uint64_t InvertedIndex::getMtime(const std::string &path) {
  struct stat st;
  if (::stat(path.c_str(), &st) == 0)
    return static_cast<uint64_t>(st.st_mtime);
  return 0;
}

/* ─────────────────────────────────────────────────
   LIFECYCLE: open / close
   ───────────────────────────────────────────────── */

void InvertedIndex::open(const std::string &dbDir) {
  int rc;

  /* 1. Create environment */
  rc = mdb_env_create(&env);
  lmdb_check(rc, "mdb_env_create");

  /* 2. Configure: 2 GB map, 6 named databases */
  rc = mdb_env_set_mapsize(env, (size_t)2 * 1024 * 1024 * 1024);
  lmdb_check(rc, "mdb_env_set_mapsize");

  rc = mdb_env_set_maxdbs(env, 5);
  lmdb_check(rc, "mdb_env_set_maxdbs");

  /* 3. Open backing directory */
  fs::create_directories(dbDir);
  rc = mdb_env_open(env, dbDir.c_str(), 0, 0664);
  if (rc != MDB_SUCCESS) {
    logMessage(std::string("LMDB: cannot open environment at ") + dbDir +
               " — " + mdb_strerror(rc));
    env = nullptr;
    return;
  }

  /* 4. Open sub-databases inside a write transaction */
  MDB_txn *txn;
  mdb_txn_begin(env, nullptr, 0, &txn);
  rc = mdb_dbi_open(txn, "files", MDB_CREATE | MDB_INTEGERKEY, &db_files);
  rc |= mdb_dbi_open(txn, "word2id", MDB_CREATE, &db_word2id);
  rc |= mdb_dbi_open(txn, "id2word", MDB_CREATE | MDB_INTEGERKEY, &db_id2word);
  rc |= mdb_dbi_open(txn, "inverted",
                     MDB_CREATE | MDB_DUPSORT | MDB_INTEGERKEY | MDB_DUPFIXED,
                     &db_inverted);
  rc |= mdb_dbi_open(txn, "forward",
                     MDB_CREATE | MDB_DUPSORT | MDB_INTEGERKEY | MDB_DUPFIXED,
                     &db_forward);

  if (rc != MDB_SUCCESS) {
    lmdb_check(rc, "open/dbi_open");
    mdb_txn_abort(txn);
    return;
  }
  mdb_txn_commit(txn);

  /* 5. Initialize WordID counter and rootDev */
  MDB_txn *rtxn;
  mdb_txn_begin(env, nullptr, MDB_RDONLY, &rtxn);
  MDB_cursor *cur;
  if (mdb_cursor_open(rtxn, db_id2word, &cur) == MDB_SUCCESS) {
    MDB_val k, v;
    if (mdb_cursor_get(cur, &k, &v, MDB_LAST) == MDB_SUCCESS) {
      if (k.mv_size == sizeof(uint32_t)) {
        next_word_id = *(uint32_t *)k.mv_data + 1;
      }
    }
    mdb_cursor_close(cur);
  }
  mdb_txn_abort(rtxn);

  // Capture root volume device for /.vol/ path resolution
  uint64_t dummy;
  getStat(app.config.indexingRoot, dummy, rootDev);

  logMessage("LMDB Inode-Index opened. RootDev=" + std::to_string(rootDev) +
             ", next_word_id=" + std::to_string(next_word_id));

  // User request: print all words once
  //   dumpWords();
}

void InvertedIndex::close() {
  if (env) {
    mdb_env_close(env);
    env = nullptr;
  }
}

/* ─────────────────────────────────────────────────
   DIFFERENTIAL SYNC SUPPORT
   ───────────────────────────────────────────────── */

static const uint64_t META_INO =
    0; // Use Inode 0 for metadata (collisions impossible)

uint64_t InvertedIndex::getLastSyncTime() {
  if (!env)
    return 0;
  MDB_txn *txn;
  if (mdb_txn_begin(env, nullptr, MDB_RDONLY, &txn) != MDB_SUCCESS)
    return 0;

  MDB_val k = {sizeof(uint64_t), (void *)&META_INO};
  MDB_val v;
  uint64_t ts = 0;
  if (mdb_get(txn, db_files, &k, &v) == MDB_SUCCESS) {
    ts = *(uint64_t *)v.mv_data;
  }
  mdb_txn_abort(txn);
  return ts;
}

void InvertedIndex::setLastSyncTime(uint64_t ts) {
  if (!env)
    return;
  MDB_txn *txn;
  if (mdb_txn_begin(env, nullptr, 0, &txn) != MDB_SUCCESS)
    return;

  MDB_val k = {sizeof(uint64_t), (void *)&META_INO};
  MDB_val v = {sizeof(uint64_t), &ts};
  mdb_put(txn, db_files, &k, &v, 0);
  mdb_txn_commit(txn);
}

/* ─────────────────────────────────────────────────
   CORE: indexPath
   ───────────────────────────────────────────────── */

void InvertedIndex::indexPath(const std::string &path, MDB_txn *externalTxn) {
  if (!env)
    return;

  uint64_t ino, dev;
  getStat(path, ino, dev);
  if (ino == 0)
    return;

  MDB_txn *txn = externalTxn;
  bool localTxn = false;
  if (!txn) {
    if (mdb_txn_begin(env, nullptr, 0, &txn) != MDB_SUCCESS)
      return;
    localTxn = true;
  }

  uint64_t mt = getMtime(path);
  MDB_val kIno = {sizeof(uint64_t), &ino};
  MDB_val vMt = {sizeof(uint64_t), &mt};
  mdb_put(txn, db_files, &kIno, &vMt, 0);

  // 2. Helper for WordID
  auto getOrCreateWordId = [&](const std::string &word) -> uint32_t {
    if (word.empty())
      return 0;
    MDB_val wk = {word.size(), (void *)word.data()};
    MDB_val wv;
    if (mdb_get(txn, db_word2id, &wk, &wv) == MDB_SUCCESS) {
      return *(uint32_t *)wv.mv_data;
    }
    // New word
    uint32_t wid = ++next_word_id;
    MDB_val idV = {sizeof(uint32_t), &wid};
    mdb_put(txn, db_word2id, &wk, &idV, 0);
    mdb_put(txn, db_id2word, &idV, &wk, 0);
    return wid;
  };

  // 3. Relationships
  auto insertRel = [&](const std::string &word) {
    uint32_t wid = getOrCreateWordId(word);
    if (wid == 0)
      return;

    MDB_val wk = {sizeof(uint32_t), &wid};
    MDB_val idV = {sizeof(uint64_t), &ino}; // word_id -> ino
    int r = mdb_put(txn, db_inverted, &wk, &idV, MDB_NODUPDATA);

    MDB_val fk = {sizeof(uint64_t), &ino};
    MDB_val fv = {sizeof(uint32_t), &wid}; // ino -> word_id
    r = mdb_put(txn, db_forward, &fk, &fv, MDB_NODUPDATA);
  };

  // Function to tokenize and index a string based on whitespace delimiters
  auto tokenizeAndIndex = [&](const std::string &str) {
    std::stringstream ss(str);
    std::string word;
    while (ss >> word) {
      bool valid = true;
      for (char ch : word) {
        unsigned char uc = static_cast<unsigned char>(ch);
        // Only allow alnum + [@#_-$&]. Any other character makes the ENTIRE
        // word invalid.
        if (!(std::isalnum(uc) || uc == '@' || uc == '#' || uc == '_' ||
              uc == '-' || uc == '$' || uc == '&')) {
          valid = false;
          break;
        }
      }
      if (valid && !word.empty()) {
        std::string clean = normalizeWord(word);
        if (!clean.empty() && clean.size() < 128) {
          insertRel(clean);
        }
      }
    }
  };

  // 1. Index filename
  size_t pos = path.find_last_of("/\\");
  std::string name = (pos == std::string::npos) ? path : path.substr(pos + 1);
  tokenizeAndIndex(name);

  // 2. Index content (only text files, non-hidden)
  if (!isDirectory(path) && isRegularFile(path) && !isBinaryFile(path)) {
    if (name.empty() || name[0] != '.') {
      std::ifstream fp(path);
      if (fp) {
        std::string line;
        while (getline(fp, line)) {
          tokenizeAndIndex(line);
        }
      }
    }
  }

  if (localTxn)
    mdb_txn_commit(txn);
}

/* ─────────────────────────────────────────────────
   CORE: removePath
   ───────────────────────────────────────────────── */

void InvertedIndex::removePath(const std::string &path, MDB_txn *externalTxn) {
  if (!env)
    return;
  uint64_t ino, dev;
  getStat(path, ino, dev);
  if (ino == 0)
    return;

  MDB_txn *txn = externalTxn;
  bool localTxn = false;
  if (!txn) {
    if (mdb_txn_begin(env, nullptr, 0, &txn) != MDB_SUCCESS)
      return;
    localTxn = true;
  }

  MDB_cursor *fwdCur;
  mdb_cursor_open(txn, db_forward, &fwdCur);
  MDB_val fk = {sizeof(uint64_t), &ino};
  MDB_val fv;

  if (mdb_cursor_get(fwdCur, &fk, &fv, MDB_SET) == MDB_SUCCESS) {
    do {
      uint32_t wid = *(uint32_t *)fv.mv_data;
      MDB_val wk = {sizeof(uint32_t), &wid};
      MDB_val idV = {sizeof(uint64_t), &ino};
      mdb_del(txn, db_inverted, &wk, &idV);
    } while (mdb_cursor_get(fwdCur, &fk, &fv, MDB_NEXT_DUP) == MDB_SUCCESS);

    mdb_cursor_get(fwdCur, &fk, &fv, MDB_SET);
    int delRc;
    do {
      delRc = mdb_cursor_del(fwdCur, 0);
    } while (delRc == MDB_SUCCESS &&
             mdb_cursor_get(fwdCur, &fk, &fv, MDB_GET_CURRENT) == MDB_SUCCESS);
  }
  mdb_cursor_close(fwdCur);

  MDB_val kIno = {sizeof(uint64_t), &ino};
  mdb_del(txn, db_files, &kIno, nullptr);
  
  if (localTxn)
    mdb_txn_commit(txn);
}

/* ─────────────────────────────────────────────────
   CORE: search  (AND semantics, read-only transaction)
   ───────────────────────────────────────────────── */

void InvertedIndex::search(const std::string &query) {
  app.search.foundPaths.clear();
  if (!env)
    return;

  std::vector<std::string> tokens;
  std::stringstream ss(query);
  std::string token;
  while (ss >> token) {
    std::string clean = normalizeWord(token);
    if (!clean.empty())
      tokens.push_back(clean);
  }
  if (tokens.empty())
    return;

  MDB_txn *txn;
  if (mdb_txn_begin(env, nullptr, MDB_RDONLY, &txn) != MDB_SUCCESS)
    return;

  std::unordered_map<uint64_t, int> fileCounter;
  for (const auto &word : tokens) {
    MDB_val wk = {word.size(), (void *)word.data()};
    MDB_val wv;
    if (mdb_get(txn, db_word2id, &wk, &wv) != MDB_SUCCESS) {
      mdb_txn_abort(txn);
      return;
    }
    uint32_t wid = *(uint32_t *)wv.mv_data;

    MDB_cursor *cur;
    mdb_cursor_open(txn, db_inverted, &cur);
    MDB_val iK = {sizeof(uint32_t), &wid};
    MDB_val iV;
    if (mdb_cursor_get(cur, &iK, &iV, MDB_SET) == MDB_SUCCESS) {
      do {
        uint64_t ino = *(uint64_t *)iV.mv_data;
        fileCounter[ino]++;
      } while (mdb_cursor_get(cur, &iK, &iV, MDB_NEXT_DUP) == MDB_SUCCESS);
    }
    mdb_cursor_close(cur);
  }
  mdb_txn_abort(txn);

  int required = (int)tokens.size();
  for (auto &[ino, cnt] : fileCounter) {
    if (cnt != required)
      continue;

    // Resolve path via volfs
    std::string volPath =
        "/.vol/" + std::to_string(rootDev) + "/" + std::to_string(ino);
    char pathBuf[4096];
    int fd = ::open(volPath.c_str(), O_RDONLY);
    if (fd >= 0) {
      if (::fcntl(fd, F_GETPATH, pathBuf) != -1) {
        std::string resolved(pathBuf);
        // Filter: ensure it starts with indexingRoot
        if (resolved.find(app.config.indexingRoot) == 0) {
          app.search.foundPaths.push_back(std::move(resolved));
        }
      }
      ::close(fd);
    }
  }
}

/* ─────────────────────────────────────────────────
   rectifyIndex  (public-facing, same API as before)
   ───────────────────────────────────────────────── */

void InvertedIndex::rectifyIndex(RectifyAction action,
                                 const std::vector<std::string> &oldPaths,
                                 const std::vector<std::string> &newPaths) {
  // Logic suspended as per user request to speed up operations.
  // Inode-indexing will be refreshed during the background crawl.
}

void InvertedIndex::dumpWords() {
  if (!env)
    return;
  MDB_txn *txn;
  if (mdb_txn_begin(env, nullptr, MDB_RDONLY, &txn) != MDB_SUCCESS)
    return;

  MDB_cursor *cur;
  if (mdb_cursor_open(txn, db_id2word, &cur) == MDB_SUCCESS) {
    MDB_val k, v;
    logMessage("--- [DEBUG] INDEXED WORDS START ---");
    while (mdb_cursor_get(cur, &k, &v, MDB_NEXT) == MDB_SUCCESS) {
      if (v.mv_size > 0) {
        std::string word((char *)v.mv_data, v.mv_size);
        logMessage("WordID " + std::to_string(*(uint32_t *)k.mv_data) + ": " +
                   word);
      }
    }
    logMessage("--- [DEBUG] INDEXED WORDS END ---");
    mdb_cursor_close(cur);
  }
  mdb_txn_abort(txn);
}