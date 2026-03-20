/*
 * inverted_index.cpp  —  LMDB-backed persistent inverted index
 *
 * Databases inside the single MDB_env:
 *   db_files    : path(str)    -> FileRecord{file_id, mtime}  (unique keys)
 *   db_inverted : word(str)    -> file_id(uint32_t)           (DUPSORT |
 * DUPFIXED) db_forward  : file_id(u32) -> word(str)                   (DUPSORT
 * | INTEGERKEY) db_id2path  : file_id(u32) -> path(str) (INTEGERKEY, unique)
 *
 * A special sentinel key "__meta_last_sync__" in db_files stores the
 * last global sync timestamp (uint64_t) for differential crawl support.
 */

#include "myheader.h"

/* ─────────────────────────────────────────────────
   INTERNAL HELPERS
   ───────────────────────────────────────────────── */

// Abort the process on unrecoverable LMDB errors
static void lmdb_check(int rc, const char *op) {
  if (rc != MDB_SUCCESS && rc != MDB_NOTFOUND) {
    logMessage(std::string("LMDB error in ") + op + ": " + mdb_strerror(rc));
    // don't abort the whole app on individual put/get errors — just log
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

  rc = mdb_env_set_maxdbs(env, 6);
  lmdb_check(rc, "mdb_env_set_maxdbs");

  /* 3. Open (or create) the backing directory + file */
  fs::create_directories(dbDir);
  rc = mdb_env_open(env, dbDir.c_str(), 0, 0664);
  if (rc != MDB_SUCCESS) {
    logMessage(std::string("LMDB: cannot open environment at ") + dbDir +
               " — " + mdb_strerror(rc));
    env = nullptr;
    return;
  }

  /* 4. Open all sub-databases inside a write transaction */
  MDB_txn *txn;
  rc = mdb_txn_begin(env, nullptr, 0, &txn);
  lmdb_check(rc, "open/mdb_txn_begin");

  rc = mdb_dbi_open(txn, "files", MDB_CREATE, &db_files);
  lmdb_check(rc, "open/db_files");

  rc = mdb_dbi_open(txn, "word2id", MDB_CREATE, &db_word2id);
  lmdb_check(rc, "open/db_word2id");

  rc = mdb_dbi_open(txn, "id2word", MDB_CREATE | MDB_INTEGERKEY, &db_id2word);
  lmdb_check(rc, "open/db_id2word");

  rc = mdb_dbi_open(txn, "inverted",
                    MDB_CREATE | MDB_DUPSORT | MDB_INTEGERKEY | MDB_DUPFIXED,
                    &db_inverted);
  lmdb_check(rc, "open/db_inverted");

  rc = mdb_dbi_open(txn, "forward",
                    MDB_CREATE | MDB_DUPSORT | MDB_INTEGERKEY | MDB_DUPFIXED,
                    &db_forward);
  lmdb_check(rc, "open/db_forward");

  rc = mdb_dbi_open(txn, "id2path", MDB_CREATE | MDB_INTEGERKEY, &db_id2path);
  lmdb_check(rc, "open/db_id2path");

  rc = mdb_txn_commit(txn);
  lmdb_check(rc, "open/commit");

  /* 5. Initialize ID counters by finding current maximums */
  MDB_txn *rtxn;
  mdb_txn_begin(env, nullptr, MDB_RDONLY, &rtxn);

  MDB_cursor *cur;
  // Max FileID
  if (mdb_cursor_open(rtxn, db_id2path, &cur) == MDB_SUCCESS) {
    MDB_val k, v;
    if (mdb_cursor_get(cur, &k, &v, MDB_LAST) == MDB_SUCCESS) {
      next_file_id = *(uint32_t *)k.mv_data + 1;
    }
    mdb_cursor_close(cur);
  }
  // Max WordID
  if (mdb_cursor_open(rtxn, db_id2word, &cur) == MDB_SUCCESS) {
    MDB_val k, v;
    if (mdb_cursor_get(cur, &k, &v, MDB_LAST) == MDB_SUCCESS) {
      next_word_id = *(uint32_t *)k.mv_data + 1;
    }
    mdb_cursor_close(cur);
  }
  mdb_txn_abort(rtxn);

  logMessage("LMDB index opened. next_file_id=" + std::to_string(next_file_id) +
             ", next_word_id=" + std::to_string(next_word_id));
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

static const char *META_SYNC_KEY = "__meta_last_sync__";

uint64_t InvertedIndex::getLastSyncTime() {
  if (!env)
    return 0;

  MDB_txn *txn;
  mdb_txn_begin(env, nullptr, MDB_RDONLY, &txn);

  MDB_val k = {strlen(META_SYNC_KEY), (void *)META_SYNC_KEY};
  MDB_val v;
  uint64_t ts = 0;

  if (mdb_get(txn, db_files, &k, &v) == MDB_SUCCESS &&
      v.mv_size == sizeof(uint64_t)) {
    ts = *(uint64_t *)v.mv_data;
  }

  mdb_txn_abort(txn);
  return ts;
}

void InvertedIndex::setLastSyncTime(uint64_t ts) {
  if (!env)
    return;

  MDB_txn *txn;
  mdb_txn_begin(env, nullptr, 0, &txn);

  MDB_val k = {strlen(META_SYNC_KEY), (void *)META_SYNC_KEY};
  MDB_val v = {sizeof(uint64_t), &ts};
  mdb_put(txn, db_files, &k, &v, 0);

  mdb_txn_commit(txn);
}

/* ─────────────────────────────────────────────────
   CORE: indexPath
   ───────────────────────────────────────────────── */

void InvertedIndex::indexPath(const std::string &path) {
  if (!env)
    return;

  MDB_txn *txn;
  int rc = mdb_txn_begin(env, nullptr, 0, &txn);
  if (rc != MDB_SUCCESS)
    return;

  /* ── 1. Get or create FileRecord for this path ── */
  MDB_val pk = {path.size(), (void *)path.data()};
  MDB_val pv;
  FileRecord rec;
  bool isNew = (mdb_get(txn, db_files, &pk, &pv) == MDB_NOTFOUND);

  if (isNew) {
    rec.file_id = next_file_id++;
    rec.mtime = getMtime(path);
  } else {
    rec = *(FileRecord *)pv.mv_data;
    rec.mtime = getMtime(path); // refresh mtime
  }

  /* ── 2. Store FileRecord (path → record) ── */
  MDB_val recVal = {sizeof(FileRecord), &rec};
  mdb_put(txn, db_files, &pk, &recVal, 0);

  /* ── 3. Store reverse lookup (file_id → path) ── */
  MDB_val idKey = {sizeof(uint32_t), &rec.file_id};
  MDB_val pathVal = {path.size(), (void *)path.data()};
  mdb_put(txn, db_id2path, &idKey, &pathVal, 0);

  /* ── 4. Helper lambda: resolve word to ID (get or create) ── */
  auto getOrCreateWordId = [&](const std::string &word) -> uint32_t {
    if (word.empty())
      return 0;

    MDB_val wk = {word.size(), (void *)word.data()};
    MDB_val wv;
    if (mdb_get(txn, db_word2id, &wk, &wv) == MDB_SUCCESS) {
      return *(uint32_t *)wv.mv_data;
    }

    // New word: assign next_word_id
    uint32_t wid = next_word_id++;
    MDB_val idKey = {sizeof(uint32_t), &wid};
    MDB_val idVal = {sizeof(uint32_t), &wid}; // for word2id value

    mdb_put(txn, db_word2id, &wk, &idVal, 0);
    mdb_put(txn, db_id2word, &idKey, &wk, 0);
    return wid;
  };

  /* ── 5. Helper lambda: insert relationship into both indices ── */
  auto insertRelationship = [&](const std::string &word) {
    uint32_t wid = getOrCreateWordId(word);
    if (wid == 0)
      return;

    // db_inverted: word_id → file_id
    MDB_val wk = {sizeof(uint32_t), &wid};
    MDB_val wv = {sizeof(uint32_t), &rec.file_id};
    mdb_put(txn, db_inverted, &wk, &wv, MDB_NODUPDATA);

    // db_forward: file_id → word_id
    MDB_val fk = {sizeof(uint32_t), &rec.file_id};
    MDB_val fv = {sizeof(uint32_t), &wid};
    mdb_put(txn, db_forward, &fk, &fv, MDB_NODUPDATA);
  };

  /* ── 6. Index the filename/dirname itself ── */
  size_t pos = path.find_last_of("/\\");
  std::string name = (pos == std::string::npos) ? path : path.substr(pos + 1);
  insertRelationship(normalizeWord(name));

  /* ── 7. If regular file: index its content ── */
  if (!isDirectory(path) && isRegularFile(path)) {
    std::ifstream fp(path);
    if (fp) {
      std::string line, word;
      while (getline(fp, line)) {
        std::stringstream ss(line);
        while (ss >> word) {
          insertRelationship(normalizeWord(word));
        }
      }
    }
  }

  rc = mdb_txn_commit(txn);
  lmdb_check(rc, "indexPath/commit");
}

/* ─────────────────────────────────────────────────
   CORE: indexAllOnce (batch from queue)
   ───────────────────────────────────────────────── */

void InvertedIndex::indexAllOnce(std::queue<std::string> &paths) {
  while (!paths.empty()) {
    std::string path = paths.front();
    paths.pop();
    indexPath(path);
  }
}

/* ─────────────────────────────────────────────────
   CORE: removePath
   Uses the Forward Index to efficiently find all words for this file,
   then removes each from the Inverted Index — O(W log N) total.
   ───────────────────────────────────────────────── */

void InvertedIndex::removePath(const std::string &path) {
  if (!env)
    return;

  MDB_txn *txn;
  int rc = mdb_txn_begin(env, nullptr, 0, &txn);
  if (rc != MDB_SUCCESS)
    return;

  /* ── 1. Look up FileRecord ── */
  MDB_val pk = {path.size(), (void *)path.data()};
  MDB_val pv;
  if (mdb_get(txn, db_files, &pk, &pv) == MDB_NOTFOUND) {
    mdb_txn_abort(txn);
    return; // not indexed, nothing to do
  }
  FileRecord rec = *(FileRecord *)pv.mv_data;

  /* ── 2. Iterate Forward Index: for each word_id this file had ── */
  MDB_cursor *fwdCur;
  mdb_cursor_open(txn, db_forward, &fwdCur);

  MDB_val fk = {sizeof(uint32_t), &rec.file_id};
  MDB_val fv;

  if (mdb_cursor_get(fwdCur, &fk, &fv, MDB_SET) == MDB_SUCCESS) {
    do {
      /* Remove (word_id → file_id) from db_inverted */
      uint32_t wid = *(uint32_t *)fv.mv_data;
      MDB_val wk = {sizeof(uint32_t), &wid};
      MDB_val wv = {sizeof(uint32_t), &rec.file_id};
      mdb_del(txn, db_inverted, &wk, &wv);

    } while (mdb_cursor_get(fwdCur, &fk, &fv, MDB_NEXT_DUP) == MDB_SUCCESS);

    /* ── 3. Delete all forward index entries for this file ── */
    mdb_cursor_get(fwdCur, &fk, &fv, MDB_SET);
    int delRc;
    do {
      delRc = mdb_cursor_del(fwdCur, 0);
    } while (delRc == MDB_SUCCESS &&
             mdb_cursor_get(fwdCur, &fk, &fv, MDB_GET_CURRENT) == MDB_SUCCESS);
  }

  mdb_cursor_close(fwdCur);

  /* ── 4. Delete path → record entry ── */
  mdb_del(txn, db_files, &pk, nullptr);

  /* ── 5. Delete file_id → path reverse entry ── */
  MDB_val idKey = {sizeof(uint32_t), &rec.file_id};
  mdb_del(txn, db_id2path, &idKey, nullptr);

  rc = mdb_txn_commit(txn);
  lmdb_check(rc, "removePath/commit");
}

/* ─────────────────────────────────────────────────
   CORE: search  (AND semantics, read-only transaction)
   ───────────────────────────────────────────────── */

void InvertedIndex::search(const std::string &query) {
  app.search.foundPaths.clear();
  if (!env)
    return;

  /* ── 1. Tokenize + normalize query ── */
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

  /* ── 2. Read-only transaction (non-blocking: MVCC allows concurrent searches)
   * ── */
  MDB_txn *txn;
  int rc = mdb_txn_begin(env, nullptr, MDB_RDONLY, &txn);
  if (rc != MDB_SUCCESS)
    return;

  /* ── 3. For each token, collect matching FileIDs into a counter ── */
  std::unordered_map<uint32_t, int> fileCounter;

  for (const auto &word : tokens) {
    /* Resolve word to word_id */
    MDB_val wk = {word.size(), (void *)word.data()};
    MDB_val wv;
    if (mdb_get(txn, db_word2id, &wk, &wv) != MDB_SUCCESS) {
      // Word not in dictionary -> AND semantics: no results possible
      mdb_txn_abort(txn);
      return;
    }
    uint32_t wid = *(uint32_t *)wv.mv_data;

    /* Query inverted index with word_id */
    MDB_cursor *cur;
    mdb_cursor_open(txn, db_inverted, &cur);

    MDB_val idKey = {sizeof(uint32_t), &wid};
    MDB_val idVal;

    if (mdb_cursor_get(cur, &idKey, &idVal, MDB_SET) != MDB_SUCCESS) {
      // No file contains this word_id
      mdb_cursor_close(cur);
      mdb_txn_abort(txn);
      return;
    }

    // Iterate all duplicate FileIDs for this word_id
    do {
      uint32_t fid = *(uint32_t *)idVal.mv_data;
      fileCounter[fid]++;
    } while (mdb_cursor_get(cur, &idKey, &idVal, MDB_NEXT_DUP) == MDB_SUCCESS);

    mdb_cursor_close(cur);
  }

  /* ── 4. Intersection: keep files matching ALL tokens ── */
  int required = (int)tokens.size();

  for (auto &[fid, cnt] : fileCounter) {
    if (cnt != required)
      continue;

    // Reverse-lookup: file_id → path  (O(log N) via db_id2path)
    MDB_val idKey = {sizeof(uint32_t), (void *)&fid};
    MDB_val idVal;
    if (mdb_get(txn, db_id2path, &idKey, &idVal) == MDB_SUCCESS) {
      std::string foundPath((char *)idVal.mv_data, idVal.mv_size);
      if (!foundPath.empty())
        app.search.foundPaths.push_back(std::move(foundPath));
    }
  }

  /* Read-only txn: abort is correct (no writes to flush) */
  mdb_txn_abort(txn);
}

/* ─────────────────────────────────────────────────
   rectifyIndex  (public-facing, same API as before)
   ───────────────────────────────────────────────── */

void InvertedIndex::rectifyIndex(RectifyAction action,
                                 const std::vector<std::string> &oldPaths,
                                 const std::vector<std::string> &newPaths) {
  switch (action) {

  case RectifyAction::CREATE:
  case RectifyAction::COPY:
    for (const auto &p : newPaths)
      indexPath(p);
    break;

  case RectifyAction::RENAME:
    // 1-to-1: oldPaths[i] → newPaths[i]
    for (size_t i = 0; i < oldPaths.size() && i < newPaths.size(); i++) {
      removePath(oldPaths[i]);
      indexPath(newPaths[i]);
    }
    break;

  case RectifyAction::DELETE:
    for (const auto &p : oldPaths)
      removePath(p);
    break;
  }
}