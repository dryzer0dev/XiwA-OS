#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_BOOKMARKS 100
#define MAX_HISTORY 1000
#define MAX_DOWNLOADS 100
#define MAX_EXTENSIONS 50

typedef struct {
    char url[1024];
    char title[256];
    time_t visit_time;
} HistoryEntry;

typedef struct {
    char name[256];
    char url[1024];
    char icon[1024];
} Bookmark;

typedef struct {
    char name[256];
    char version[32];
    char description[512];
    int is_enabled;
} Extension;

typedef struct {
    char url[1024];
    char filename[256];
    size_t size;
    size_t downloaded;
    time_t start_time;
    int status; 
} Download;

typedef struct {
    HistoryEntry history[MAX_HISTORY];
    int history_count;
    Bookmark bookmarks[MAX_BOOKMARKS];
    int bookmark_count;
    Extension extensions[MAX_EXTENSIONS];
    int extension_count;
    Download downloads[MAX_DOWNLOADS];
    int download_count;
    char current_url[1024];
    char search_engine[256];
} WebBrowser;

WebBrowser* init_browser() {
    WebBrowser* browser = (WebBrowser*)malloc(sizeof(WebBrowser));
    if (!browser) return NULL;

    browser->history_count = 0;
    browser->bookmark_count = 0;
    browser->extension_count = 0;
    browser->download_count = 0;
    strcpy(browser->search_engine, "https://www.google.com/search?q=");
    return browser;
}

int navigate_to(WebBrowser* browser, const char* url) {
    strncpy(browser->current_url, url, 1024);
    
    if (browser->history_count < MAX_HISTORY) {
        HistoryEntry* entry = &browser->history[browser->history_count];
        strncpy(entry->url, url, 1024);
        strncpy(entry->title, "Page Web", 256); 
        entry->visit_time = time(NULL);
        browser->history_count++;
    }

    printf("Navigation vers: %s\n", url);
    return 0;
}

int search_google(WebBrowser* browser, const char* query) {
    char search_url[2048];
    snprintf(search_url, sizeof(search_url), "%s%s", browser->search_engine, query);
    return navigate_to(browser, search_url);
}

int add_bookmark(WebBrowser* browser, const char* name, const char* url) {
    if (browser->bookmark_count >= MAX_BOOKMARKS) return -1;

    Bookmark* bookmark = &browser->bookmarks[browser->bookmark_count];
    strncpy(bookmark->name, name, 256);
    strncpy(bookmark->url, url, 1024);
    strncpy(bookmark->icon, "🌐", 1024); // Icône par défaut

    browser->bookmark_count++;
    return 0;
}

int install_extension(WebBrowser* browser, const char* name, const char* version,
                     const char* description) {
    if (browser->extension_count >= MAX_EXTENSIONS) return -1;

    Extension* ext = &browser->extensions[browser->extension_count];
    strncpy(ext->name, name, 256);
    strncpy(ext->version, version, 32);
    strncpy(ext->description, description, 512);
    ext->is_enabled = 1;

    browser->extension_count++;
    return 0;
}

int download_file(WebBrowser* browser, const char* url, const char* filename) {
    if (browser->download_count >= MAX_DOWNLOADS) return -1;

    Download* download = &browser->downloads[browser->download_count];
    strncpy(download->url, url, 1024);
    strncpy(download->filename, filename, 256);
    download->size = 0; // À déterminer lors du téléchargement
    download->downloaded = 0;
    download->start_time = time(NULL);
    download->status = 1; // En cours

    browser->download_count++;
    return 0;
}

void install_popular_extensions(WebBrowser* browser) {
    install_extension(browser, "AdBlock", "1.0.0",
                     "Bloque les publicités sur les sites web");

    install_extension(browser, "Dark Mode", "1.0.0",
                     "Active le mode sombre sur tous les sites");

    install_extension(browser, "Google Translate", "1.0.0",
                     "Traduit automatiquement les pages web");

    install_extension(browser, "Password Manager", "1.0.0",
                     "Gère et sécurise vos mots de passe");
}

void show_history(WebBrowser* browser) {
    printf("\nHistorique de navigation:\n");
    printf("URL\t\t\tTitre\t\t\tDate de visite\n");
    printf("----------------------------------------------------------------------\n");

    for (int i = 0; i < browser->history_count; i++) {
        HistoryEntry* entry = &browser->history[i];
        char time_str[20];
        strftime(time_str, 20, "%Y-%m-%d %H:%M", localtime(&entry->visit_time));
        printf("%s\t%s\t%s\n", entry->url, entry->title, time_str);
    }
} 