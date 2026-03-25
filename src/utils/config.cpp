#include "myheader.hpp"

void loadConfig() {
    std::ifstream file(CONFIG_PATH);
    if (!file.is_open()) {
        logMessage("config.json not found, using default workers = 4");
        app.config.workers = 4;
        app.config.indexing_enabled = false;
        app.config.indexing_root = "/home";
        return;
    }

    try {
        json cfg;
        file >> cfg;

        if (cfg.contains("performance")) {
            auto& perf = cfg["performance"];
            
            if (perf.contains("workers")) {
                app.config.workers = perf["workers"].get<int>();
                if (app.config.workers <= 0) app.config.workers = 4;
            } else {
                app.config.workers = 4;
            }

            if (perf.contains("indexing")) {
                app.config.indexing_enabled = perf["indexing"].get<bool>();
            } else {
                app.config.indexing_enabled = false;
            }

            if (perf.contains("indexing_root")) {
                app.config.indexing_root = perf["indexing_root"].get<std::string>();
            } else {
                app.config.indexing_root = "/home";
            }
        }

        logMessage("Configuration loaded successfully from config.json");
        logMessage("Workers: " + std::to_string(app.config.workers));
        logMessage("Indexing: " + std::string(app.config.indexing_enabled ? "ENABLED" : "DISABLED"));
        logMessage("Indexing Root: " + app.config.indexing_root);

    } catch (const std::exception& e) {
        logMessage("Error parsing config.json: " + std::string(e.what()));
        logMessage("Using default configuration");
        app.config.workers = 4;
        app.config.indexing_enabled = false;
        app.config.indexing_root = "/home";
    }

    file.close();
}
