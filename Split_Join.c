#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


// Fonction pour découper un fichier en parties
void splitFile(const char *srcFile, size_t partSize) {
    int fdSrc = open(srcFile, O_RDWR); // O_RDWR pour pouvoir lire et modifier le fichier
    if (fdSrc == -1) {
        perror("ERREUR: Ouverture du fichier source");
        exit(1);
    }

    // Récupérer la taille du fichier source
    struct stat stFichierSrc;
    if (fstat(fdSrc, &stFichierSrc) == -1) {
        perror("ERREUR: Récupération des informations du fichier source");
        close(fdSrc);
        exit(1);
    }

    off_t fileSize = stFichierSrc.st_size; // Taille totale du fichier
    char buffer[1024];                    // Tampon pour lire par blocs
    ssize_t bytesRead, bytesWritten;

    int partNum = 1;
    while (fileSize > 0) {

        // Générer un nom pour chaque partie
        char partName[256];
        snprintf(partName, sizeof(partName), "%s.part%d", srcFile, partNum);

        int fdDst = open(partName, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fdDst == -1) {
            perror("ERREUR: Création du fichier de destination");
            close(fdSrc);
            exit(1);
        }

        size_t bytesRemaining = partSize; // Taille à écrire pour cette partie
        while (bytesRemaining > 0 && fileSize > 0) {
            size_t bytesToRead = bytesRemaining < sizeof(buffer) ? bytesRemaining : sizeof(buffer);
            bytesRead = read(fdSrc, buffer, bytesToRead);
            if (bytesRead == -1) {
                perror("ERREUR: Lecture du fichier source");
                close(fdSrc);
                close(fdDst);
                exit(1);
            }

            if (bytesRead == 0) { // Fin du fichier
                break;
            }

            bytesWritten = write(fdDst, buffer, bytesRead);
            if (bytesWritten != bytesRead) {
                perror("ERREUR: Écriture dans le fichier de destination");
                close(fdSrc);
                close(fdDst);
                exit(1);
            }

            bytesRemaining -= bytesWritten; // Réduire la taille restante pour cette partie
            fileSize -= bytesWritten;       // Réduire la taille restante du fichier
        }

        close(fdDst);
        partNum++;
    }

    // Vider le contenu du fichier source
    if (ftruncate(fdSrc, 0) == -1) {
        perror("ERREUR: Impossible de vider le fichier source");
        close(fdSrc);
        exit(1);
    }

    close(fdSrc);
    printf("Fichier découpé avec succès en %d parties, et le fichier source a été vidé.\n", partNum - 1);
}

// Fonction pour rejoindre les fichiers découpés
void joinFiles(int numParts, char *parts[], const char *dstFile) {
    int fdDst = open(dstFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fdDst == -1) {
        perror("ERREUR: Création du fichier de destination");
        exit(1);
    }

    char buffer[1024];
    ssize_t bytesRead, bytesWritten;

    for (int i = 0; i < numParts; i++) {
        int fdSrc = open(parts[i], O_RDONLY);
        if (fdSrc == -1) {
            perror("ERREUR: Ouverture du fichier source");
            close(fdDst);
            exit(1);
        }

        while ((bytesRead = read(fdSrc, buffer, sizeof(buffer))) > 0) {
            bytesWritten = write(fdDst, buffer, bytesRead);
            if (bytesWritten != bytesRead) {
                perror("ERREUR: Écriture dans le fichier de destination");
                close(fdSrc);
                close(fdDst);
                exit(1);
            }
        }

        close(fdSrc);
    }

    close(fdDst);
    printf("Fichiers joints avec succès dans %s\n", dstFile);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Utilisation pour découper: ./split fichier_source taille_partie\n");
        fprintf(stderr, "Utilisation pour joindre : ./split fichier_final part1 part2 ...\n");
        return 1;
    }

    if (argc == 3) { // Cas de découpage
        const char *srcFile = argv[1];
        size_t partSize = atoi(argv[2]);

        if (partSize <= 0) {
            perror("La taille maximale de la partie doit être un entier positif.");
            return 1;
        }

        splitFile(srcFile, partSize);
    } else { // Cas de recombinaison
        const char *dstFile = argv[1];
        int numParts = argc - 2;
        char *parts[numParts];

        for (int i = 0; i < numParts; i++) {
            parts[i] = argv[i + 2];
        }

        joinFiles(numParts, parts, dstFile);
    }

    return 0;
}




