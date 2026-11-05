#ifndef ATA_H
#define ATA_H
int  ata_init(void);
int  ata_present(void);
int  ata_read_lba (unsigned int lba, unsigned char count, void *buf);
int  ata_write_lba(unsigned int lba, unsigned char count, const void *buf);
#endif