#include "x-watcher.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#define SIGNAL_FILE "/data/adb/copg_config_updated"

static volatile sig_atomic_t keep_running = 1;

void signal_handler(int sig) {
    keep_running = 0;
}

void config_changed_callback(XWATCHER_FILE_EVENT event, const char *path, 
                           int context, void *additional_data) {
    if (event == XWATCHER_FILE_MODIFIED || event == XWATCHER_FILE_CREATED) {
        FILE *fp = fopen(SIGNAL_FILE, "w");
        if (fp) {
            fputc('1', fp);  
            fclose(fp);
            chmod(SIGNAL_FILE, 0644);  
            sync();
            printf("Config file %s modified, signaled shell script\n", path);
        } else {
            fprintf(stderr, "Failed to write to signal file: %s\n", strerror(errno));
        }
    }
}

int main() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    x_watcher *watcher = xWatcher_create();
    if (!watcher) {
        fprintf(stderr, "Failed to create watcher\n");
        return 1;
    }

    xWatcher_reference config_ref = {
        .path = "/data/adb/modules/COPG/config.json",
        .callback_func = config_changed_callback,
        .context = 1,
        .additional_data = NULL
    };

    if (!xWatcher_appendFile(watcher, &config_ref)) {
        fprintf(stderr, "Failed to add watcher for config.json\n");
        xWatcher_destroy(watcher);
        return 1;
    }

    printf("Monitoring %s for changes...\n", config_ref.path);
    if (!xWatcher_start(watcher)) {
        fprintf(stderr, "Failed to start watcher\n");
        xWatcher_destroy(watcher);
        return 1;
    }

    while (keep_running) {
        pause();  
    }

    printf("Shutting down gracefully...\n");
    xWatcher_destroy(watcher);
    return 0;
}
