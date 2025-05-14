#include <gtk/gtk.h>
#include <json-c/json.h>
#include <stdio.h>
#include <string.h>

// Structure pour les paramètres
typedef struct {
    char theme[32];
    int transparency;
    char font[32];
    int font_size;
    char accent_color[16];
    int animations;
    char language[8];
    int dark_mode;
    int sidebar_position;
    int start_menu_style;
} Settings;

// Charger les paramètres depuis le fichier JSON
void load_settings(Settings *settings) {
    struct json_object *json;
    json = json_object_from_file("/usr/local/share/xiwa/settings.json");
    
    if (json) {
        struct json_object *theme, *transparency, *font, *font_size;
        struct json_object *accent_color, *animations, *language;
        struct json_object *dark_mode, *sidebar_pos, *start_menu;
        
        json_object_object_get_ex(json, "theme", &theme);
        json_object_object_get_ex(json, "transparency", &transparency);
        json_object_object_get_ex(json, "font", &font);
        json_object_object_get_ex(json, "font_size", &font_size);
        json_object_object_get_ex(json, "accent_color", &accent_color);
        json_object_object_get_ex(json, "animations", &animations);
        json_object_object_get_ex(json, "language", &language);
        json_object_object_get_ex(json, "dark_mode", &dark_mode);
        json_object_object_get_ex(json, "sidebar_position", &sidebar_pos);
        json_object_object_get_ex(json, "start_menu_style", &start_menu);
        
        strcpy(settings->theme, json_object_get_string(theme));
        settings->transparency = json_object_get_int(transparency);
        strcpy(settings->font, json_object_get_string(font));
        settings->font_size = json_object_get_int(font_size);
        strcpy(settings->accent_color, json_object_get_string(accent_color));
        settings->animations = json_object_get_int(animations);
        strcpy(settings->language, json_object_get_string(language));
        settings->dark_mode = json_object_get_int(dark_mode);
        settings->sidebar_position = json_object_get_int(sidebar_pos);
        settings->start_menu_style = json_object_get_int(start_menu);
        
        json_object_put(json);
    }
}

// Sauvegarder les paramètres dans le fichier JSON
void save_settings(Settings *settings) {
    struct json_object *json = json_object_new_object();
    
    json_object_object_add(json, "theme", json_object_new_string(settings->theme));
    json_object_object_add(json, "transparency", json_object_new_int(settings->transparency));
    json_object_object_add(json, "font", json_object_new_string(settings->font));
    json_object_object_add(json, "font_size", json_object_new_int(settings->font_size));
    json_object_object_add(json, "accent_color", json_object_new_string(settings->accent_color));
    json_object_object_add(json, "animations", json_object_new_int(settings->animations));
    json_object_object_add(json, "language", json_object_new_string(settings->language));
    json_object_object_add(json, "dark_mode", json_object_new_int(settings->dark_mode));
    json_object_object_add(json, "sidebar_position", json_object_new_int(settings->sidebar_position));
    json_object_object_add(json, "start_menu_style", json_object_new_int(settings->start_menu_style));
    
    json_object_to_file("/usr/local/share/xiwa/settings.json", json);
    json_object_put(json);
}

// Créer l'interface des paramètres
GtkWidget* create_settings_window(void) {
    Settings settings;
    load_settings(&settings);
    
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Paramètres XiwA OS");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
    
    GtkWidget *notebook = gtk_notebook_new();
    gtk_container_add(GTK_CONTAINER(window), notebook);
    
    // Onglet Apparence
    GtkWidget *appearance = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), appearance, gtk_label_new("Apparence"));
    
    // Thème
    GtkWidget *theme_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(appearance), theme_box, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(theme_box), gtk_label_new("Thème:"), FALSE, FALSE, 5);
    
    GtkWidget *theme_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(theme_combo), "dark", "Sombre");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(theme_combo), "light", "Clair");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(theme_combo), "custom", "Personnalisé");
    gtk_box_pack_start(GTK_BOX(theme_box), theme_combo, TRUE, TRUE, 5);
    
    // Transparence
    GtkWidget *transparency_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(appearance), transparency_box, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(transparency_box), gtk_label_new("Transparence:"), FALSE, FALSE, 5);
    
    GtkWidget *transparency_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
    gtk_scale_set_value_pos(GTK_SCALE(transparency_scale), GTK_POS_RIGHT);
    gtk_box_pack_start(GTK_BOX(transparency_box), transparency_scale, TRUE, TRUE, 5);
    
    // Onglet Personnalisation
    GtkWidget *customization = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), customization, gtk_label_new("Personnalisation"));
    
    // Police
    GtkWidget *font_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(customization), font_box, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(font_box), gtk_label_new("Police:"), FALSE, FALSE, 5);
    
    GtkWidget *font_button = gtk_font_button_new();
    gtk_box_pack_start(GTK_BOX(font_box), font_button, TRUE, TRUE, 5);
    
    // Couleur d'accent
    GtkWidget *color_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(customization), color_box, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(color_box), gtk_label_new("Couleur d'accent:"), FALSE, FALSE, 5);
    
    GtkWidget *color_button = gtk_color_button_new();
    gtk_box_pack_start(GTK_BOX(color_box), color_button, TRUE, TRUE, 5);
    
    // Onglet Système
    GtkWidget *system = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), system, gtk_label_new("Système"));
    
    // Langue
    GtkWidget *language_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(system), language_box, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(language_box), gtk_label_new("Langue:"), FALSE, FALSE, 5);
    
    GtkWidget *language_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(language_combo), "fr", "Français");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(language_combo), "en", "English");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(language_combo), "es", "Español");
    gtk_box_pack_start(GTK_BOX(language_box), language_combo, TRUE, TRUE, 5);
    
    // Boutons d'action
    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_end(GTK_BOX(window), button_box, FALSE, FALSE, 5);
    
    GtkWidget *apply_button = gtk_button_new_with_label("Appliquer");
    GtkWidget *cancel_button = gtk_button_new_with_label("Annuler");
    GtkWidget *ok_button = gtk_button_new_with_label("OK");
    
    gtk_box_pack_end(GTK_BOX(button_box), ok_button, FALSE, FALSE, 5);
    gtk_box_pack_end(GTK_BOX(button_box), cancel_button, FALSE, FALSE, 5);
    gtk_box_pack_end(GTK_BOX(button_box), apply_button, FALSE, FALSE, 5);
    
    return window;
} 