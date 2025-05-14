#include <gtk/gtk.h>
#include <glib.h>
#include <string.h>

// Structure pour stocker les informations de la fenêtre
typedef struct {
    GtkWidget *window;
    GtkWidget *titlebar;
    GtkWidget *menu_bar;
    GtkWidget *status_bar;
    GtkWidget *sidebar;
    GtkWidget *main_area;
} WindowData;

// Fonction pour créer la barre de titre personnalisée
GtkWidget* create_titlebar(WindowData *data) {
    GtkWidget *titlebar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_style_context_add_class(gtk_widget_get_style_context(titlebar), "titlebar");
    
    // Logo système
    GtkWidget *logo = gtk_image_new_from_icon_name("system-logo", GTK_ICON_SIZE_MENU);
    gtk_box_pack_start(GTK_BOX(titlebar), logo, FALSE, FALSE, 5);
    
    // Titre de la fenêtre
    GtkWidget *title = gtk_label_new("XiwA OS");
    gtk_box_pack_start(GTK_BOX(titlebar), title, TRUE, TRUE, 0);
    
    // Boutons de contrôle (style Windows 12)
    GtkWidget *min_button = gtk_button_new_from_icon_name("window-minimize-symbolic", GTK_ICON_SIZE_MENU);
    GtkWidget *max_button = gtk_button_new_from_icon_name("window-maximize-symbolic", GTK_ICON_SIZE_MENU);
    GtkWidget *close_button = gtk_button_new_from_icon_name("window-close-symbolic", GTK_ICON_SIZE_MENU);
    
    gtk_box_pack_end(GTK_BOX(titlebar), close_button, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(titlebar), max_button, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(titlebar), min_button, FALSE, FALSE, 0);
    
    return titlebar;
}

// Fonction pour créer la barre latérale (style Ubuntu)
GtkWidget* create_sidebar(WindowData *data) {
    GtkWidget *sidebar = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_style_context_add_class(gtk_widget_get_style_context(sidebar), "sidebar");
    
    // Boutons de la barre latérale
    const char *icons[] = {
        "system-search-symbolic",
        "user-home-symbolic",
        "folder-documents-symbolic",
        "applications-system-symbolic",
        "preferences-system-symbolic"
    };
    
    const char *labels[] = {
        "Rechercher",
        "Accueil",
        "Documents",
        "Applications",
        "Paramètres"
    };
    
    for (int i = 0; i < 5; i++) {
        GtkWidget *button = gtk_button_new();
        GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        
        GtkWidget *icon = gtk_image_new_from_icon_name(icons[i], GTK_ICON_SIZE_MENU);
        GtkWidget *label = gtk_label_new(labels[i]);
        
        gtk_box_pack_start(GTK_BOX(box), icon, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);
        gtk_container_add(GTK_CONTAINER(button), box);
        gtk_box_pack_start(GTK_BOX(sidebar), button, FALSE, FALSE, 5);
    }
    
    return sidebar;
}

// Fonction pour créer la barre de menu (style Windows 12)
GtkWidget* create_menu_bar(WindowData *data) {
    GtkWidget *menu_bar = gtk_menu_bar_new();
    
    // Menu Fichier
    GtkWidget *file_menu = gtk_menu_item_new_with_label("Fichier");
    GtkWidget *file_submenu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(file_menu), file_submenu);
    
    GtkWidget *new_item = gtk_menu_item_new_with_label("Nouveau");
    GtkWidget *open_item = gtk_menu_item_new_with_label("Ouvrir");
    GtkWidget *save_item = gtk_menu_item_new_with_label("Enregistrer");
    GtkWidget *exit_item = gtk_menu_item_new_with_label("Quitter");
    
    gtk_menu_shell_append(GTK_MENU_SHELL(file_submenu), new_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_submenu), open_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_submenu), save_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_submenu), gtk_separator_menu_item_new());
    gtk_menu_shell_append(GTK_MENU_SHELL(file_submenu), exit_item);
    
    gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), file_menu);
    
    return menu_bar;
}

// Fonction pour créer la fenêtre principale
GtkWidget* create_main_window(void) {
    WindowData *data = g_malloc(sizeof(WindowData));
    
    // Création de la fenêtre principale
    data->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(data->window), "XiwA OS");
    gtk_window_set_default_size(GTK_WINDOW(data->window), 1024, 768);
    
    // Conteneur principal
    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(data->window), main_box);
    
    // Ajout des composants
    data->titlebar = create_titlebar(data);
    data->menu_bar = create_menu_bar(data);
    data->sidebar = create_sidebar(data);
    
    gtk_box_pack_start(GTK_BOX(main_box), data->titlebar, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(main_box), data->menu_bar, FALSE, FALSE, 0);
    
    // Zone principale avec sidebar
    GtkWidget *content_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(main_box), content_box, TRUE, TRUE, 0);
    
    gtk_box_pack_start(GTK_BOX(content_box), data->sidebar, FALSE, FALSE, 0);
    
    // Zone de contenu principale
    data->main_area = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(content_box), data->main_area, TRUE, TRUE, 0);
    
    // Barre de statut
    data->status_bar = gtk_statusbar_new();
    gtk_box_pack_end(GTK_BOX(main_box), data->status_bar, FALSE, FALSE, 0);
    
    return data->window;
}

// Fonction principale
int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    
    // Chargement du thème personnalisé
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_path(provider, "gui/style.css", NULL);
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );
    
    GtkWidget *window = create_main_window();
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    
    gtk_widget_show_all(window);
    gtk_main();
    
    return 0;
} 