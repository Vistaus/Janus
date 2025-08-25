#include <gtksourceview/gtksource.h>
#include <gdk/gdkkeysyms.h>
#include "global.h"
#include "commands.h"
#include "init.h"
#include <locale.h>
#include "config.h"

int main(int argc, char * argv[]) {

    gtk_init(NULL, NULL);
    gtk_source_init();

    GtkWidget * window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Janus");

    // Create main text buffer
    GtkWidget * text = gtk_source_view_new();
    GtkWidget * view = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(view), text);
    GtkTextBuffer * buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text));
    gtk_source_buffer_set_highlight_syntax(GTK_SOURCE_BUFFER(buffer), FALSE);

    struct Document document = {
        .buffer = buffer,
        .view = text,
        .window = GTK_WINDOW(window),
        .last = gtk_source_region_new(buffer),
    };

    // Connect singals
    g_signal_connect(window, "delete-event", G_CALLBACK(delete_event), &document);
    g_signal_connect(buffer, "modified-changed", G_CALLBACK(change_indicator), &document);

    // Preferences setup
    char path[PATH_MAX] = "";
    if (getenv("APPDIR") && strlen(getenv("APPDIR")) < PATH_MAX - strlen("/usr/share/locale/"))
        strcpy(path, getenv("APPDIR"));
    strcat(path, "/usr/share/locale/");

    setlocale(LC_ALL, "");
    bindtextdomain("janus", path);
    bind_textdomain_codeset("janus", "utf-8");
    textdomain("janus");

    gtk_window_set_icon_name(GTK_WINDOW(window), "dev.pantheum.janus");

    strcpy(path, (strlen(g_get_user_config_dir()) < PATH_MAX - strlen(CONFIG_FILE)) ? g_get_user_config_dir() : "~/.config");
    strcat(path, CONFIG_FILE);

    GError * error = NULL;
    GKeyFile * config = g_key_file_new();

    // Only an error if the file exists but could not load a key from it. Also only scans file if it exists because the and skips otherwise.
    if (!access(path, F_OK) && !g_key_file_load_from_file(config, path, G_KEY_FILE_NONE, &error)) {
        g_log(G_LOG_DOMAIN, G_LOG_LEVEL_WARNING, "%s", error->message);
        g_error_free(error);
    }

    int height = g_key_file_get_integer(config, GROUP_KEY, "height", NULL);
    int width = g_key_file_get_integer(config, GROUP_KEY, "width", NULL);
    gtk_window_set_default_size(GTK_WINDOW(window), width ? width : DEFAULT_WIDTH, height ? height : DEFAULT_HEIGHT);

    document.font = g_key_file_get_string(config, GROUP_KEY, "font", NULL);
    set_font(&document);

    // Menu setup
    GtkAccelGroup * accel = gtk_accel_group_new();
    gtk_window_add_accel_group(GTK_WINDOW(window), accel);
    GtkWidget * bar = gtk_menu_bar_new();
    init_menu(bar, accel, &document, config);

    g_key_file_free(config);

    // Search setup
    search_init(&document);

    // Boxes
    GtkWidget * box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(box), bar, 0, 0, 0);
    gtk_box_pack_start(GTK_BOX(box), view, 1, 1, 0);

    // Pack up app and run
    gtk_container_add(GTK_CONTAINER(window), box);
    gtk_widget_show_all (window);

    // Open any files
    if (argc > 1)
        open_file(&document, g_file_new_for_commandline_arg(argv[1]), FALSE);
    else if (!isatty(STDIN_FILENO)) // In case user tries to pipe text
        open_file(&document, g_file_new_for_path("/dev/stdin"), TRUE);

    gtk_main();

    gtk_source_finalize();
    g_free(document.font);

    return 0;
}
