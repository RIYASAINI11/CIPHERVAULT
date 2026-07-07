#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MAGIC "CVLT"
#define VERSION 1
typedef struct
{
    char magic[4];
    uint8_t version;
    uint32_t passwordHash;
} FileHeader;
uint32_t hashPassword(const char *password)
{
    uint32_t hash = 5381;

    while(*password)
    {
        hash=((hash<<5)+hash)+*password;
        password++;
    }

    return hash;
}
void xorData(unsigned char *buffer,long size,uint32_t key)
{
    for(long i=0;i<size;i++)
    {
        buffer[i]^=((key>>(8*(i%4)))&0xFF);
    }
}
int encryptFile(const char *inputFile,
                const char *outputFile,
                const char *password)
{
    FILE *fin = fopen(inputFile, "rb");

    if(fin == NULL)
    {
        printf("Error: Cannot open input file.\n");
        return 0;
    }

    FILE *fout = fopen(outputFile, "wb");

    if(fout == NULL)
    {
        fclose(fin);
        printf("Error: Cannot create output file.\n");
        return 0;
    }

    /* Calculate password hash */

    FileHeader header;

    memcpy(header.magic, MAGIC, 4);

    header.version = VERSION;

    header.passwordHash = hashPassword(password);

    /* Write header */

    fwrite(&header,sizeof(FileHeader),1,fout);

    /* Find file size */

    fseek(fin,0,SEEK_END);

    long fileSize = ftell(fin);

    rewind(fin);

    if(fileSize<=0)
    {
        fclose(fin);
        fclose(fout);

        printf("Empty file.\n");

        return 0;
    }

    /* Allocate memory */

    unsigned char *buffer=(unsigned char*)malloc(fileSize);

    if(buffer==NULL)
    {
        fclose(fin);
        fclose(fout);

        printf("Memory allocation failed.\n");

        return 0;
    }

    fread(buffer,1,fileSize,fin);

    /* Encrypt */

    xorData(buffer,fileSize,header.passwordHash);

    fwrite(buffer,1,fileSize,fout);

    free(buffer);

    fclose(fin);

    fclose(fout);

    printf("\n=============================\n");
    printf(" Encryption Successful\n");
    printf("=============================\n");
    printf("Input  : %s\n",inputFile);
    printf("Output : %s\n",outputFile);

    return 1;
}
int decryptFile(const char *inputFile,
                const char *outputFile,
                const char *password)
{
    FILE *fin = fopen(inputFile, "rb");

    if(fin == NULL)
    {
        printf("Error: Cannot open encrypted file.\n");
        return 0;
    }

    FILE *fout = fopen(outputFile, "wb");

    if(fout == NULL)
    {
        fclose(fin);
        printf("Error: Cannot create output file.\n");
        return 0;
    }

    /* Read Header */

    FileHeader header;

    if(fread(&header, sizeof(FileHeader), 1, fin) != 1)
    {
        printf("Invalid encrypted file.\n");

        fclose(fin);
        fclose(fout);

        return 0;
    }

    /* Verify Magic */

    if(memcmp(header.magic, MAGIC, 4) != 0)
    {
        printf("This is not a CipherVault encrypted file.\n");

        fclose(fin);
        fclose(fout);

        return 0;
    }

    /* Verify Version */

    if(header.version != VERSION)
    {
        printf("Unsupported file version.\n");

        fclose(fin);
        fclose(fout);

        return 0;
    }

    /* Verify Password */

    uint32_t enteredHash = hashPassword(password);

    if(enteredHash != header.passwordHash)
    {
        printf("\n=============================\n");
        printf(" Incorrect Password\n");
        printf("=============================\n");

        fclose(fin);
        fclose(fout);

        remove(outputFile);

        return 0;
    }

    /* Read Remaining Data */

    fseek(fin,0,SEEK_END);

    long fileSize = ftell(fin);

    long dataSize = fileSize - sizeof(FileHeader);

    rewind(fin);

    fseek(fin,sizeof(FileHeader),SEEK_SET);

    unsigned char *buffer = (unsigned char*)malloc(dataSize);

    if(buffer == NULL)
    {
        printf("Memory allocation failed.\n");

        fclose(fin);
        fclose(fout);

        return 0;
    }

    fread(buffer,1,dataSize,fin);

    /* Decrypt */

    xorData(buffer,dataSize,enteredHash);

    fwrite(buffer,1,dataSize,fout);

    free(buffer);

    fclose(fin);

    fclose(fout);

    printf("\n=============================\n");
    printf(" Decryption Successful\n");
    printf("=============================\n");
    printf("Input  : %s\n",inputFile);
    printf("Output : %s\n",outputFile);

    return 1;
}
int main(int argc, char *argv[])
{
    if(argc != 5)
    {
        printf("\n=====================================\n");
        printf("        CipherVault Utility\n");
        printf("=====================================\n");

        printf("\nUsage:\n");

        printf("Encrypt:\n");
        printf("cipher.exe encrypt inputFile outputFile password\n\n");

        printf("Decrypt:\n");
        printf("cipher.exe decrypt inputFile outputFile password\n\n");

        return 1;
    }

    char *mode = argv[1];
    char *inputFile = argv[2];
    char *outputFile = argv[3];
    char *password = argv[4];

    if(strlen(password) < 4)
    {
        printf("Password must be at least 4 characters long.\n");
        return 1;
    }

    if(strcmp(mode,"encrypt")==0)
    {
        if(encryptFile(inputFile,outputFile,password))
        {
            printf("\nEncryption Completed Successfully.\n");
            return 0;
        }

        return 1;
    }

    else if(strcmp(mode,"decrypt")==0)
    {
        if(decryptFile(inputFile,outputFile,password))
        {
            printf("\nDecryption Completed Successfully.\n");
            return 0;
        }

        return 1;
    }

    else
    {
        printf("Invalid Mode.\n");
        printf("Use encrypt or decrypt.\n");
        return 1;
    }
}