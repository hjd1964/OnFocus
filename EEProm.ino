// EEPROM Extensions

// write long numbers into EEPROM
void EEPROM_writeLong(int i,long l) {
  byte *lPtr;
  lPtr = (byte*)&l;
  EEPROM.write(i+0,*lPtr); lPtr++;
  EEPROM.write(i+1,*lPtr); lPtr++;
  EEPROM.write(i+2,*lPtr); lPtr++;
  EEPROM.write(i+3,*lPtr);
}

// read long numbers from EEPROM
long EEPROM_readLong(int i) {
  long l;
  byte *lPtr;
  lPtr = (byte*)&l;
  *lPtr=EEPROM.read(i+0); lPtr++;
  *lPtr=EEPROM.read(i+1); lPtr++;
  *lPtr=EEPROM.read(i+2); lPtr++;
  *lPtr=EEPROM.read(i+3);  
  return l;
}
