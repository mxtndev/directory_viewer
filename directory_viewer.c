#include <gtk/gtk.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

typedef struct {
    GtkTreeStore *store;
    GtkTreeIter *parent_iter;
} TreeData;

// Функция для рекурсивного обхода директории
void populate_tree(const char *path, TreeData *tree_data) {
    DIR *dir = opendir(path);
    if (!dir) {
        perror("opendir");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Пропускаем специальные директории "." и ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Формируем полный путь к элементу
        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        // Получаем информацию о файле/директории
        struct stat statbuf;
        if (stat(full_path, &statbuf) != 0) {
            perror("stat");
            continue;
        }

        // Создаем новую строку в модели
        GtkTreeIter iter;
        gtk_tree_store_append(tree_data->store, &iter, tree_data->parent_iter);
        gtk_tree_store_set(tree_data->store, &iter, 0, entry->d_name, 1, full_path, -1);

        // Если это директория, рекурсивно обходим её
        if (S_ISDIR(statbuf.st_mode)) {
            TreeData sub_tree_data = {tree_data->store, &iter};
            populate_tree(full_path, &sub_tree_data);
        }
    }

    closedir(dir);
}

// Callback-функция для создания интерфейса
static void activate(GtkApplication *app, gpointer user_data) {
    // Создаем окно
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Directory Viewer");
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);

    // Создаем TreeView
    GtkWidget *treeview = gtk_tree_view_new();
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(
        "Files and Directories", renderer, "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

    // Создаем TreeStore с двумя столбцами: имя и полный путь
    GtkTreeStore *store = gtk_tree_store_new(2, G_TYPE_STRING, G_TYPE_STRING);

    // Получаем текущую рабочую директорию
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("getcwd");
        return;
    }

    // Заполняем дерево
    TreeData tree_data = {store, NULL};
    populate_tree(cwd, &tree_data);

    // Устанавливаем модель для TreeView
    gtk_tree_view_set_model(GTK_TREE_VIEW(treeview), GTK_TREE_MODEL(store));
    g_object_unref(store);

    // Добавляем TreeView в окно
    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scrolled_window), treeview);
    gtk_container_add(GTK_CONTAINER(window), scrolled_window);

    // Отображаем все виджеты
    gtk_widget_show_all(window);
}

int main(int argc, char **argv) {
    GtkApplication *app = gtk_application_new("org.example.directoryviewer", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}