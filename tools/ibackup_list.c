#include <libibackup/libibackup.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <plist/plist.h>

#ifdef __cplusplus
extern "C" {
#endif

const char* file_type_string_for_type(uint32_t type) {
    switch (type) {
        case IBACKUP_FLAG_FILE:
            return "F";
        case IBACKUP_FLAG_DIRECTORY:
            return "D";
        case IBACKUP_FLAG_SYMBOLIC_LINK:
            return "S";
    }
    return "U";
}

void get_file_path(libibackup_client_t client, char* file_id) {
    char* file_path = libibackup_get_path_for_file_id(client, file_id);

    printf("Full Path: %s\n", file_path);

    libibackup_free(file_path);
}

void list_domains(libibackup_client_t client) {
    libibackup_domain_metrics metrics;
    printf("Listing Domains\n");
    char **domain_list;
    libibackup_list_domains(client, &domain_list);

    int64_t index = 0;
    while (domain_list[index] != NULL) {
        libibackup_get_domain_metrics(client, domain_list[index], &metrics);
        printf("Domain: %s (%d files, %d directories, %d symlinks)\n", domain_list[index],
               metrics.file_count, metrics.directory_count, metrics.symlink_count);

        free(domain_list[index]);
        index++;
    }

    free(domain_list);
}

void list_files(libibackup_client_t client, char* domain) {
    printf("Listing files for domain %s\n", domain);
    libibackup_file_entry_t **file_list;
    libibackup_file_metadata metadata;


    libibackup_list_files_for_domain(client, domain, &file_list);

    int64_t index = 0;
    while (file_list[index] != NULL) {
        libibackup_get_metadata_by_id(client, file_list[index]->file_id, &metadata);
        switch (file_list[index]->type) {
            case IBACKUP_FLAG_FILE:
                printf("%s: [%s] %s (size %llu)\n", file_list[index]->file_id, file_type_string_for_type(file_list[index]->type),
                       file_list[index]->relative_path, metadata.size);
                break;
            case IBACKUP_FLAG_DIRECTORY:
                printf("%s: [%s] %s\n", file_list[index]->file_id, file_type_string_for_type(file_list[index]->type),
                       file_list[index]->relative_path);
                break;
            case IBACKUP_FLAG_SYMBOLIC_LINK:
                printf("%s: [%s] %s -> %s\n", file_list[index]->file_id, file_type_string_for_type(file_list[index]->type),
                       file_list[index]->relative_path, metadata.target);
                break;
        }

        index++;
    }

    free(file_list);
}

void remove_file(libibackup_client_t client, char* file_id) {
    libibackup_remove_file_by_id(client, file_id);
}

void get_file_metadata(libibackup_client_t client, char* file_id) {
    plist_t metadata;
    char* xml;
    uint32_t length;
    libibackup_get_raw_metadata_by_id(client, file_id, &metadata);

    plist_to_xml(metadata, &xml, &length);

    printf("Metadata\n%s\n", xml);
}

void check_files(libibackup_client_t client) {
    struct stat file_stat;
    char* file_path = malloc(24);
    char* combined_path;
    printf("Checking for broken files\n");
    libibackup_file_entry_t **file_list;
    char **domain_list;
    libibackup_list_domains(client, &domain_list);

    int64_t domain_index = 0;
    while (domain_list[domain_index] != NULL) {
        libibackup_list_files_for_domain(client, domain_list[domain_index], &file_list);

        int64_t file_index = 0;
        while (file_list[file_index] != NULL) {
            combined_path = libibackup_get_path_for_file_id(client, file_path);

            if (stat(combined_path, &file_stat) != 0) {
                printf("Broken File at path %s", combined_path);
            }

            free(combined_path);
            file_index++;
        }

        printf("Scanned %lld files in domain %s\n", file_index, domain_list[domain_index]);

        free(file_list);

        free(domain_list[domain_index]);
        domain_index++;
    }
    free(file_path);
    free(domain_list);
}

void list_all_files(libibackup_client_t client) {
    char* file_path = malloc(24);
    libibackup_file_entry_t **file_list;
    libibackup_file_metadata metadata;
    char **domain_list;
    libibackup_list_domains(client, &domain_list);

    int64_t domain_index = 0;
    while (domain_list[domain_index] != NULL) {
        printf("Files in domain %s\n", domain_list[domain_index]);
        libibackup_list_files_for_domain(client, domain_list[domain_index], &file_list);

        int64_t file_index = 0;
        while (file_list[file_index] != NULL) {
            if (file_list[file_index]->type != IBACKUP_FLAG_DIRECTORY) {
                libibackup_get_metadata_by_id(client, file_list[file_index]->file_id, &metadata);
            }
            switch (file_list[file_index]->type) {
                case IBACKUP_FLAG_FILE:
                    printf("%s: [%s] %s (size %llu)\n", file_list[file_index]->file_id, file_type_string_for_type(file_list[file_index]->type),
                           file_list[file_index]->relative_path, metadata.size);
                    break;
                case IBACKUP_FLAG_DIRECTORY:
                    printf("%s: [%s] %s\n", file_list[file_index]->file_id, file_type_string_for_type(file_list[file_index]->type),
                           file_list[file_index]->relative_path);
                    break;
                case IBACKUP_FLAG_SYMBOLIC_LINK:
                    printf("%s: [%s] %s -> %s\n", file_list[file_index]->file_id, file_type_string_for_type(file_list[file_index]->type),
                           file_list[file_index]->relative_path, metadata.target);
                    break;
            }

            file_index++;
        }

        free(file_list);

        free(domain_list[domain_index]);
        domain_index++;
    }
    free(file_path);
    free(domain_list);
}

void get_file(libibackup_client_t client, char* file_id) {
    char* file_path;
    struct stat path_stat;

    libibackup_get_file_by_id(client, file_id, &file_path);
    stat(file_path, &path_stat);

    FILE* file = fopen(file_path, "r");
    if (file == NULL) {
        printf("Unable to read file\n");
        return;
    }
    void* data = malloc(path_stat.st_size);
    printf("Read file with size %lld\n", path_stat.st_size);

    fread(data, path_stat.st_size, 1, file);
    fclose(file);

    if (strncmp(data, "bplist00", 8) == 0) {
        plist_t data_plist;
        char* xml_plist;
        uint32_t size;
        plist_from_memory(data, path_stat.st_size, &data_plist);
        plist_to_xml(data_plist, &xml_plist, &size);
        write(STDOUT_FILENO, xml_plist, size);
    }
    else {
        write(STDOUT_FILENO, data, path_stat.st_size);
    }
}

int main(int argc, char **argv) {
    //libibackup_set_debug(true);
    if (argc < 3) {
        printf("Invalid Arguments\n");
        return -1;
    }

    libibackup_client_t client;

    libibackup_open_backup(argv[2], &client);

    printf("Backup Opened\n");


    if (strcmp(argv[1], "list_domains") == 0) {
        list_domains(client);
    }
    else if (strcmp(argv[1], "list_files") == 0) {
        list_files(client, argv[3]);
    }
    else if (strcmp(argv[1], "get_file_metadata") == 0) {
        get_file_metadata(client, argv[3]);
    }
    else if (strcmp(argv[1], "get_file") == 0) {
        get_file(client, argv[3]);
    }
    else if (strcmp(argv[1], "remove_file") == 0) {
        remove_file(client, argv[3]);
    }
    else if (strcmp(argv[1], "check_files") == 0) {
        check_files(client);
    }
    else if (strcmp(argv[1], "list_all_files") == 0) {
        list_all_files(client);
    }
    else if (strcmp(argv[1], "get_file_path") == 0) {
        get_file_path(client, argv[3]);
    }
    else {
        printf("Unknown command %s\n", argv[1]);
    }

    libibackup_close(client);

    return 0;
}

#ifdef __cplusplus
}
#endif