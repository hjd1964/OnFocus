// Command processing

byte bufferPtr_serial_zero=0;
boolean serial_zero_ready=false;
boolean commandError;
boolean quietReply;
char command[3];
char parameter[17];
char reply[32];
char command_serial_zero[23];
char parameter_serial_zero[17];

// process commands
void processCommands() {
    boolean supress_frame = false;
    char *conv_end;
    long i=0;

    if ((Serial.available() > 0) && (!serial_zero_ready)) { serial_zero_ready = buildCommand(Serial.read()); }

    byte process_command = 0;
    if (serial_zero_ready) { strcpy(command,command_serial_zero); strcpy(parameter,parameter_serial_zero); serial_zero_ready=false; clearCommand_serial_zero(); process_command=1; } 
    else return;
    
    if (process_command) {

// Command is two chars followed by an optional parameter...
      commandError=false;
      quietReply=false;
// Handles empty and one char replies
      reply[0]=0; reply[1]=0;

//   M - Move Commands
      if (command[0]=='M') {
//  :MH#  Move Halt
//         Returns: no reply
        if (command[1]=='H') {
          targetPos=currentPos;
          quietReply=true;
        } else
//  :MIn#  Move In by n steps
//         Returns: no reply
        if (command[1]=='I') {
          if (atol2((char*)&parameter[0],&i)) {
            targetPos-=i;
            if (targetPos<0) targetPos=0;
            quietReply=true;
          } else commandError=true;
        } else
//  :MOn#  Move Out by n steps
//         Returns: no reply
        if (command[1]=='O') {
          if (atol2((char*)&parameter[0],&i)) {
            targetPos+=i;
            if (targetPos>maxPos) targetPos=maxPos;
            quietReply=true;
          } else commandError=true;
        } else
//  :MAn#  Move to Absolute position n
//         Returns: no reply
        if (command[1]=='A') {
          if (atol2((char*)&parameter[0],&i)) {
            if (i<0) i=0;
            if (i>maxPos) i=maxPos;
            targetPos=i;
            quietReply=true;
          } else commandError=true;
        } else commandError=true;
      } else

//   I - Info Commands
      if (command[0]=='I') {
//  :IS#  get status
//         Returns: M# (for moving) or S# (for stopped)
        if (command[1]=='S') {
          quietReply=true;
          if (moving) strcpy(reply,"M"); else strcpy(reply,"S");
        } else
//  :IN#  get version Number
//         Returns: s#
        if (command[1]=='N') {
          strcpy(reply,FirmwareNumber);
          quietReply=true;
        } else
//  :IP#  get Product
//         Returns: s#
        if (command[1]=='P') {
          strcpy(reply,FirmwareName);
          quietReply=true;
        } else commandError=true;
      } else

//   S - Set Commands
      if (command[0]=='S') {
//  :SZ#  Set Zero position (current position)
//         Returns: no reply
        if (command[1]=='Z') {
          targetPos=0;
          currentPos=0;
          EEPROM_writeLong(base+EE_target,targetPos);
          EEPROM_writeLong(base+EE_target,currentPos);
          quietReply=true;
        } else
//  :SH#  Set Half travel position (current position)
//         Returns: no reply
        if (command[1]=='H') {
          targetPos=maxPos/2;
          currentPos=maxPos/2;
          EEPROM_writeLong(base+EE_target,targetPos);
          EEPROM_writeLong(base+EE_target,currentPos);
          quietReply=true;
        } else
//  :SMn#  Set max position
//         Returns: no reply
        if (command[1]=='M') {
          if (atol2((char*)&parameter[0],&i)) {
            if (i<0) i=0;
            maxPos=i;
            EEPROM_writeLong(base+EE_max,maxPos);
            quietReply=true;
          } else commandError=true;
        } else commandError=true;
      } else 
      
//   G - Get Commands
      if (command[0]=='G') {
//  :GI#  Get full in position
//         Returns: 0#
        if (command[1]=='I') {
          sprintf(reply,"%ld",0);
          quietReply=true;
        } else
//  :GM#  Get max position
//         Returns: n#
        if (command[1]=='M') {
          sprintf(reply,"%ld",maxPos);
          quietReply=true;
        } else
//  :GP#  Get position
//         Returns: n#
        if (command[1]=='P') {
          sprintf(reply,"%ld",currentPos);
          quietReply=true;
        } else
//  :GS#  Get scale
//         Returns: n.nnn#
        if (command[1]=='S') {
          dtostrf(MICROS_PER_STEP,1,3,reply);
          quietReply=true;
        } else commandError=true;
      } else commandError=true;
    }

    if (!quietReply) {
      if (commandError) reply[0]='0'; else reply[0]='1';
      reply[1]=0;
      supress_frame=true;
    }
    
    if (strlen(reply)>0) {

    if (process_command==1) {
#ifdef CHKSUM0_ON
      // calculate the checksum
      char HEXS[3]="";
      byte cks=0; for (int cksCount0=0; cksCount0<strlen(reply); cksCount0++) {  cks+=reply[cksCount0]; }
      sprintf(HEXS,"%02X",cks);
      strcat(reply,HEXS);
#endif
      strcat(reply,"#");
      Serial.print(reply);
    }

    }
    quietReply=false;
    
  }

// Build up a command
boolean buildCommand(char c) {
  // ignore spaces/lf/cr, dropping spaces is another tweek to allow compatibility with LX200 protocol
  if ((c!=(char)32) && (c!=(char)10) && (c!=(char)13) && (c!=(char)6)) {
    command_serial_zero[bufferPtr_serial_zero]=c;
    bufferPtr_serial_zero++;
    command_serial_zero[bufferPtr_serial_zero]=(char)0;
    if (bufferPtr_serial_zero>22) { bufferPtr_serial_zero=22; }  // limit maximum command length to avoid overflow, c2+p16+cc2+eol2+eos1=23 bytes max ranging from 0..22
  }
  
  if (c=='#') {
    // validate the command frame, normal command
    if ((bufferPtr_serial_zero>1) && (command_serial_zero[0]==':') && (command_serial_zero[bufferPtr_serial_zero-1]=='#')) { command_serial_zero[bufferPtr_serial_zero-1]=0; } else { clearCommand_serial_zero(); return false; }

#ifdef CHKSUM0_ON
    // checksum the data, for example ":11111126".  I don't include the command frame in the checksum.  The error response is a checksumed null string "00#" which means re-transmit.
    byte len=strlen(command_serial_zero);
    byte cks=0; for (int cksCount0=1; cksCount0<len-2; cksCount0++) {  cks+=command_serial_zero[cksCount0]; }
    char chkSum[3]; sprintf(chkSum,"%02X",cks); if (!((chkSum[0]==command_serial_zero[len-2]) && (chkSum[1]==command_serial_zero[len-1]))) { clearCommand_serial_zero();  Serial.print("00#"); return false; }
    --len; command_serial_zero[--len]=0;
#endif

    // break up the command into a two char command and the remaining parameter
    
    // the parameter can be up to 16 chars in length
    memmove(parameter_serial_zero,(char *)&command_serial_zero[3],17);

    // the command is either one or two chars in length
    command_serial_zero[3]=0;  memmove(command_serial_zero,(char *)&command_serial_zero[1],3);

    return true;
  } else {
    return false;
  }
}

// clear commands
boolean clearCommand_serial_zero() {
  bufferPtr_serial_zero=0;
  command_serial_zero[bufferPtr_serial_zero]=(char)0;
  return true;
}

// integer numeric conversion with error checking
boolean atol2(char *a, long *i) {
  char *conv_end;
  long l=strtol(a,&conv_end,10);
  
  if ((l<0) || (l>10000000) || (&a[0]==conv_end)) return false;
  *i=l;
  return true;
}

