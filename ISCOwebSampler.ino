/*
This software is licensed under LGPL V3 license.
Users are encouraged to use, enhance, and extend this software but attribution is requested.

"ISCOwebSampler" connectivity software written by Ruben Kertesz 
Published on Feb 28, 2015
Designed to be built for Spark Core or a compatible device using arduino or wiring-type toolset.
This software is not affiliated with the ISCO company in any way.

This software allows a user to issue commands to an autosampler over the web (using spark.io) and receive status messages over the web.
There are some bugs occasionally. Testing is welcome.
Use of this software may lead to harm or failure of the microcontroller hardware, that of the autosampler, or it may otherwise result in damage. 
No warranty is expressed or implied.
*/

char inSerial[200] = "empty"; // this will hold the serial received from the sampler
int nStr; // used for holding the place in the serial recieve buffer array
String stsArName[6]; // used for holding variable name
int stsArVal[6]; // used for holding corresponding value

char stsMessage[40];// = {}; // hold readable status message
char stsBot[13]; //= {}; // readable bottle number
char stsVol[18]; //= {};    // readable volume                
char errMessage[25];// = {}; // readable error message

bool PoweredOn;

/* This function is called once at start up ----------------------------------*/
void setup()
{
  //Expose variables to the cloud
  Spark.variable("strReceived", inSerial, STRING);
  Spark.variable("statResponse", stsMessage, STRING);
  Spark.variable("botResponse", stsBot, STRING);
  Spark.variable("volResponse", stsVol, STRING);
  Spark.variable("errResponse", errMessage, STRING);

  //Register Spark function in the cloud
  Spark.function("connect", ISCOConnect); // if 1 is returned then we connected
  Spark.function("status", ISCOStatus); 
  Spark.function("control", ISCOControl);
  Spark.function("clear", ISCOClear);

  Serial1.begin(9600); //comm with isco
  Serial.begin(9600); //comm with computer
//  currentime = millis();
  nStr=0;
  PoweredOn = true;
}


/* This function loops forever --------------------------------------------*/
void loop()
{
//   if(stbyflag == true){
//    spitout();
//  }
}

// This function allows shutdown sentence to be presented to user
/* void spitout(void){
  if (millis() > currentime + 15000){
    Serial.println("spitout");
    if (Serial1.available()){
      while (Serial1.available() > 0) {
        int inByte = Serial1.read();
        Serial.write(inByte);  // need to send "???" to initiate communication
      }
    }
    currentime = millis();
  } 
}*/



int ISCOClear(String s)
{
  for(int i=0;i<200;i++){
    inSerial[i] = '\0';  // Is there a cleaner way of "zeroing" an array? 
  }                      // I am doing it just to be especially cautious.
  nStr=0;
}


/*******************************************************************************
 * Function Name  : ISCOConnect
 * Description    : Sends connect messages to the ISCO and feeds back responses
 *******************************************************************************/
int ISCOConnect(String s)
{
//  stbyflag = false;
    ISCOClear("a");
    
  char myarray[] = {'?','?','?','\r','\n','\0'}; 
  // I don't use char myarray[] = ???\r\n\0"; It doesn't seem to escape correctly
  
  // can I just get away with Serial1.write(myarray); instead of iterating??
  for(int n=0; n < sizeof(myarray); n++) 
  {
    Serial1.write(myarray[n]);
  }

  Serial.println("are we connected?");

 for(int i=0;i<3;i++){
       Serial.println(i);
        call(); // get the serial response
        Serial1.flush();
        Serial1.write('\r');
        delay(200);
} 



  if (Check_Protocol() == 1) { 
    Serial.println("we are Connected!");
//    stbyflag = true;
    return 1; /// if we return 1 then we know everything is ready to proceed.
  }
  else { 
    Serial.println("NOT Connected!");
//    stbyflag = true;
    return -1; //failure to connect or return ***s
  }
}

void call(void) 
{
 char subSerial[200]; // perhaps I shouldn't dimension it? I thought it wouldn't take up too much memory so I did anyway.
 int m=0;
 int o=0;
 if (Serial1.available() > 0)   // This seems redundant, yes. makes it so check protocol isn't called continuously
 {
    while (Serial1.available() > 0) {
        delay(15);
     //  199 leaves space for a last null character in case the array fills up
     // this is 199 characters not mem "slot" 199 (it is actually slot 198).
        m=Serial1.readBytes(subSerial,199);  
    }

//I have to keep calling "call" to populate the array because often, the autosampler will not provide 
//all the data before the microcontroller no longer sees serial1 as available, even with the delay.
//If I make the delay even longer, it tends to truncate data at the end of the string for some reason.
//I tried doing the looping inside call but for some reason, it did not work. I had to get out of scope,
//then come back in.

    for(int k=0;k<m;k++){ 
        o=k+nStr;                       //temp variable to use later. Includes the current position in the array (nStr)
        if (o<199){                     // is there a cleaner way than holding the current position in the string as a public
            inSerial[o]=subSerial[k];   //variable? Maybe strlen? or sizeof(if not dimensioned previously)?
            inSerial[o+1]='\0';         //Null
        }
    }
   Serial.println(inSerial); ///////////////USEFUL
   nStr=o+1;    //I stuck the null terminator on there. I'd rather not get rid of it until 
                //the next cycle (if there is one). When k=0, the next cycle of chars will
                //start right over the top of nStr, which will obliterate the null value
                //when this function completes, the final null value (after being called many times) will stay in tact
 }

 /*  if (nStr >= 199){
    Serial.println("may have exceeded capacity to store characters");
  } */

}

int Check_Protocol(void)
{   
  int testing = -1;
  Serial.println("Check_Protocol");

  char *s;
  s = strstr(inSerial, "***");
  Serial.println ("String to check for stars");
  Serial.println(s);

  if (s != NULL)    //not a null string // this is different than a null pointer, correct?
  {                 //because that would be *s (the one character @beginning of mem), vs s (the whole "string"), no?
    testing = (s - inSerial);   //this is pointer arithmetic. I think that s is pointing later in memory(?)
                                // than the pointer to inSerial which starts at the "0" part of that memory address O_o 
  }                             // index of * in buffer can be found by pointer subtraction
  else
  {
    Serial.println("NULL*");
  }

  Serial.println(testing);
  if(testing > -1) {
    Serial.println("Check protocol --> got*");
    delay(15);
    return 1;
  }
  else{
    return -1;
  }
}


/*******************************************************************************
 * Function Name  : ISCOStatus
 * Description    : receives status messages from the ISCO
 *******************************************************************************/
int ISCOStatus(String s)
{
        
//  stbyflag=false;

ISCOClear("a");

typedef struct /// I am using a struct without really understanding the definition. otherwise can't use if statement
{              // with arrays. Probably has to do with the compiler not liking me changing the value of the pointer??? 
    char c[6];
} Components;  // I don't understand the syntax here...why is this necesssary?
Components myarray;

if(PoweredOn == true){
  myarray=(Components){'S','T','S',',','1','\r'}; // it's already on
}
else{
  myarray=(Components){'S','T','S',',','2','\r'}; // not powered on and want to connect
}  

for(int n=0; n < sizeof(myarray); n++)  // can I just use Serial1.write(myarray);??
{
  Serial1.write(myarray.c[n]);
}

Serial1.flush();//waits for transmission of outgoing serial data to complete

Serial.println(sizeof(myarray));
Serial.println("what is the status?");


for(int i=0;i<3;i++){
       Serial.println(i);
        call(); // get the serial response
        Serial1.flush();
        Serial1.write('\r');
        delay(200);
}

/*Serial.println("0...");
      call(); // get the serial response
            Serial1.flush();
            Serial1.write('\r');
delay(200);
Serial.println("1");
      call();
            Serial1.flush();
            Serial1.write('\r');
delay(200);
Serial.println("2");
      call();
            Serial1.flush();
            Serial1.write('\r');
delay(200);
Serial.println("3");
     call();
            Serial1.flush();
            Serial1.write('\r');
delay(200);
Serial.println("4");
      call();
            Serial1.flush();
            Serial1.write('\r');
delay(200);
Serial.println("5");
      call();
            Serial1.flush();
            Serial1.write('\r');
delay(200);
Serial.println("6");
      call();
  delay(100);
*/
  
  if (Parse_Protocol() == 1) {
    Serial.println("DataParse Complete");
//    stbyflag=true;
      if (DisplayMessage() == 1) {
      //delay(200);
       Serial.println("Done");
//    stbyflag=true;
      return 1;
      }
  }
  else{
    Serial.println("DataParse incomplete");
//    stbyflag=true;
    return -1;
  }

}

int Parse_Protocol(void){
        
  //maybe first check for location of "ID ", send remaining chars
  //into a temporary char array and then start parsing
    char parsedStatus[15][15];//holds pointers in part of an array
            // each one of these is actually a pointer which will hold
            // the delimited parts of the response message
                Serial.println(inSerial);

    char *TI;
    TI = strstr(inSerial, "TI,"); //skip model...I'll be using 6712s
    if (TI != NULL)                     // if succesfull then
    {
        int endpos = -1;
        char *CS;
        CS = strstr(inSerial, "CS"); 
        if (CS != NULL) {
            endpos = (CS - TI); // not sure why it's backwards
            if(endpos > -1) {
                Serial.println(inSerial);
                Serial.println(CS);
                Serial.println(TI);
              Serial.println ("looks like status string is complete and in right direction/prob not repeated");
              // THERE MUST BE A MORE ELEGANT WAY TO CHECK FOR NAMES AND THEN POPULATE AN ARRAY WITH THE VALUES.
              // THE CURRENT METHOD IS RISKY. WHAT IF THERE IS A CLUSTERMESS
              // AND THE OUTPUT IS CUTOFF AND THEN REPEATED (CAN HAPPEN)
              // THEN I SHOULD START AT THE END AND WORK MY WAY BACK (RECURSIVE) RATHER 
              // THAN READ FORWARD: TI <-- STS <-- STI <-- BTL <-- SVO <-- SOR <-- CS.
              // NEED HELP TO FIGURE OUT HOW TO DO THIS - CHAR? STRING? POINTER AWERSOMENESS
              // Then again, I am indirectly checking for this by checking the names in the stsArName below. 
              // If a value is out of place, the wrong name or a jumbled mess is likely to appear there.

             // char *str;
              int counter=0;
              //while ((str = strtok_r(TI, ",", &TI)) != NULL) { // delimiter is the comma
                    //non-reentrant version at http://forum.arduino.cc/index.php/topic,59813.0.html              
                    //str gives me all of the text until the first (or next) comma
    //TRYING NON REENTRANT VERSION TO SEE IF IT HELPS
                char *token = strtok(TI, ",");
                if(token)
                {
                    delay(100);
    //*******************THIS IS CAUSING ME MAJOR HEADACHES*********************
    // RED FLASHING LIGHT. ONE RED FLASH
    //Does strcpy take toooo much memory?
    //The string I am manipulating looks like this
    //TI,41764.88008,STS,1,STI,41764.60713,BTL,5,SVO,1000,SOR,3,CS,4761
                   strncpy(parsedStatus[counter++],token,14); // You've got to COPY the data pointed to
                    //Serial.println(parsedStatus[counter]);    
    // IF I COMMENT OUT STRCPY, IT DOESN'T CRASH
                    delay(100);
                   token = strtok(NULL, ","); // Keep parsing the same string
                   while(token != NULL && counter < 15)
                   {
                   strncpy(parsedStatus[counter++],token,14); // You've got to COPY the data pointed to
                    //Serial.println(parsedStatus[counter]);
                      //OLD strcpy(parsedStatus[counter++],token); // You've got to COPY the data pointed to
                      token = strtok(NULL, ",");
                    //OLD Serial.println (parsedStatus[counter]); // We should be seeing complete words here, not one character
                      //Serial.println(token);
                   }
                }
                //  Serial.println(counter);
               // strcpy(parsedStatus[counter],str); //I am copying the X# of chars into the pointer????
                  //This is the part I am really confused about. Why would I take the entire length of 
                  //characters from memory (as found via pointer), then copy them and put them in another array of pointers? 
                  // One nice thing about duplication is that I can then take the results and save them to memory for
                  // recall later, which I probably will do in future, more autonomous versions... :-)
                  // We certainly would want to keep track of when we've sampled into which bottles, for example.
                //  Serial.println(str);

                 // ++counter; //increment the counter
                 // TI = NULL; // need null to keep parsing same string http://forum.arduino.cc/index.php/topic,48925.0.htm
                Serial.println("we made it");
              
          
              delay(700);
              //Log all the parsed data into arrays*******************
              //String temp0(parsedStatus[0]); //TI
              stsArName[0]= parsedStatus[0];
             // Serial.println(parsedStatus[0]);
             // Serial.println(stsArName[0]);
              stsArVal[0]=int(86400*(atof(parsedStatus[1]))-2208988800); //Time ************ uses fractional hour lost as int
                    //is it better to use strtod?? or atof?? not sure which
            //  Serial.println(parsedStatus[1]);
            //  Serial.println(stsArVal[0]);
              
                         
              stsArName[1]=parsedStatus[2]; //STS
           //  Serial.println(parsedStatus[2]);
           //   Serial.println(stsArName[1]);
              stsArVal[1]=atoi(parsedStatus[3]); //status
            //  Serial.println(parsedStatus[3]);
           //   Serial.println(stsArVal[1]);
              
             
             // String temp(parsedStatus[4]); //            
              stsArName[2]=parsedStatus[4]; //STI
          //   Serial.println(parsedStatus[4]);
          //    Serial.println(stsArName[2]);
              stsArVal[2]=int(86400*(atof(parsedStatus[5]))-2208988800); //sample time converted
          //    Serial.println(parsedStatus[5]);
          //    Serial.println(stsArVal[2]);
              
              //String temp(parsedStatus[6]); //            
              stsArName[3]=parsedStatus[6]; //BTL
         //    Serial.println(parsedStatus[6]);
         //     Serial.println(stsArName[3]);              
              stsArVal[3]=atoi(parsedStatus[7]); //bottlenumber
         //     Serial.println(parsedStatus[7]);
          //    Serial.println(stsArVal[3]);
              
             // String temp(parsedStatus[8]); //            
              stsArName[4]=parsedStatus[8]; //SVO
          //    Serial.println(parsedStatus[8]);
           //   Serial.println(stsArName[4]);                
              stsArVal[4]=atoi(parsedStatus[9]); //volume
         //     Serial.println(parsedStatus[9]);
          //    Serial.println(stsArVal[4]);

            //  String temp(parsedStatus[10]); //            
              stsArName[5]=parsedStatus[10]; //SOR
         //     Serial.println(parsedStatus[10]);
          //    Serial.println(stsArName[5]);              
              stsArVal[5]=atoi(parsedStatus[11]); //results 
         //     Serial.println(parsedStatus[11]);
         //     Serial.println(stsArVal[5]);
              
        //       for(int i=0;i<6;i++){
        // //    Serial.println(i);
        //       Serial.println(stsArName[i]);
        //       Serial.println(stsArVal[i]);
        //       }
              Serial.println("passedgauntlet");
              delay(400);
              return 1;
                            Serial.println("1error");
           } 
        }
        else //CS = NULL
        {
          Serial.println("CS=NULL");
                                      Serial.println("2error");
          return -1;
                                      Serial.println("3error");
        }                       // index of "" in buff can be found by pointer subtraction
    }
    else
    {
      Serial.println("TI=NULL");
                                 Serial.println("4error");
      return -2;
                                  Serial.println("5error");
    }
}

int DisplayMessage (void){
                            Serial.println("begintodisplaymessage");
//Start out status message cleared
 memset(&stsMessage[0], 0, sizeof(stsMessage));
 memset(&stsBot[0], 0, sizeof(stsBot));
 memset(&stsVol[0], 0, sizeof(stsVol));
 memset(&errMessage[0], 0, sizeof(errMessage));
 

              // counter 0 = TI
              // counter 1 = time number ... shown in a date-time format based on the number of days since
                        //00:00:00 1-Jan-1900, and the time shown as a fraction.
                 //      GET TO EPOCH days --> seconds + decimal time (fraction of a day) --> seconds
                 //       NUMBER * 86400 (<--24*60*60) - 2208988800
                        
              // counter 2 = STS
              // counter 3 = status message, 1-23
                  /*
                    1                    = WAITING TO SAMPLE.
                    4                    = POWER FAILED (for short time after power is restored).
                    5                    = PUMP JAMMED (must be resolved before continuing).
                    6                    = DISTRIBUTOR JAMMED (must be resolved before continuing).
                    9                    = SAMPLER OFF.
                    12                    = SAMPLE IN PROGRESS.
                    20                    = INVALID COMMAND. Possible causes may be:
                            · identifier code is not supported.
                            · bottle requested is not in current configuration
                            · sample volume requested is outside its range (10 - 9990 ml)
                            · day (Set_Time) must be 5 digits and more recent than 1977
                    21                    = CHECKSUM MISMATCH. (see “Optional check-sum” on page 7-8)
                    22                    = INVALID BOTTLE. (bottle requested is not in the current configuration)
                    23                    = VOLUME OUT OF RANGE. (the sample volume requested is outside its range (10-9990 ml)
                  */

                if (stsArName[1] == "STS"){ //I really should do something to strip out any errant spaces or other characters before doing comparisons.
                                          //However, the entire structure should be space free and if it has a space then something is wrong.

                    switch (stsArVal[1]) {
                        case 1:
                          //do something when var equals 1
                          strcat (stsMessage, "1= WAITING TO SAMPLE");
//                          stsMessage= "1= WAITING TO SAMPLE";
                          break;
                        case 4:
                          strcat (stsMessage, "4= POWER FAILED (after pow restored)");
                          break;
                        case 5:
                          strcat (stsMessage, "5= PUMP JAMMED (resolve)");
                          break;
                        case 6:
                          strcat (stsMessage, "6= DISTRIBUTOR JAMMED (resolve)");
                          break;
                        case 9:
                          strcat (stsMessage,  "9= SAMPLER OFF");
                          PoweredOn = false; // assume its powered on until proven false
                          break;
                        case 12:
                          strcat (stsMessage,  "12= SAMPLE IN PROGRESS");
                          break;
                        case 20:
                          strcat (stsMessage, "20= INVALID COMMAND. bot#? vol?");
                          break;
                        case 21:
                          strcat (stsMessage,  "21= CHECKSUM MISMATCH");
                          break;  
                        case 22:
                          strcat (stsMessage,  "22= INVALID BOTTLE");
                          break;   
                        case 23:
                          strcat (stsMessage,  "23= VOLUME OUT OF RANGE");
                          break; 
                        default: 
                          // if nothing else matches, do the default
                          // default is optional
                          strcat (stsMessage, "ERROR: no status value");
//                          stsMessage= "ERROR: no status value";
                      }
                    // if (stsArVal[1] == 9){
                    //     PoweredOn=false;
                    // }
                    delay(200);
                    Serial.println(stsMessage);
                }
                    else
                {
                  Serial.println("STS=NULL");
                  //statusChar = stMessage;
                  return -1;
                }
                

              // counter 4 = STI
              // counter 5 = most recent sample time
              // counter 6 = BTL
              // counter 7 = number of the most recentbottle that got water
                if (stsArName[3] == "BTL") {//I really should do something to strip out any errant spaces or other characters before doing comparisons.
                    delay(200);

                 strcat (stsBot, "bottle");
                 char str2[5]={};
                 sprintf(str2, "%d", stsArVal[3]);
                 strcat (stsBot, str2);
                 //stMessage+= (stsArVal[3]);
                 Serial.println(stsBot);
                }
                else
                {
                  Serial.println("BTL=NULL");
                 // statusChar = stMessage;
                  return -1;
                }

              // counter 8 = SVO
              // counter 9 = most recent sample volue
                      delay(100);
                      
                if (stsArName[4] == "SVO") {//I really should do something to strip out any errant spaces or other characters before doing comparisons.
                    delay(200);

                 strcat (stsVol, "volume");
                 char str1[5]={};
                 sprintf(str1, "%d", stsArVal[4]);
                 strcat (stsVol, str1);
                 Serial.println(stsVol);              
                }
                 else
                {
                  Serial.println("SVO=NULL");
                  return -1;
                }
              // counter 10 = SOR
              // counter 11 = results of most recent sampling attempt
                    
                    //0= SAMPLE OK
                    //1= NO LIQUID FOUND
                    //2= LIQUID LOST (not enough liquid)
                    //3= USER STOPPED (using the Stop Key)
                    //4= POWER FAILED
                    //5= PUMP JAMMED
                    //6= DISTRIBUTOR JAMMED
                    //8= PUMP LATCH OPEN
                    //9= SAMPLER SHUT OFF (while sampling)
                    //11= NO DISTRIBUTOR
                    //12= SAMPLE IN PROGRESS
                    
                
                                    delay(100);
                                    
                if (stsArName[5] == "SOR"){ //I really should do something to strip out any errant spaces or other characters before doing comparisons.
                                          //However, the entire structure should be space free and if it has a space then something is wrong.
                    switch (stsArVal[5]) {
                        case 0:
                          //do something when var equals 1
                          strcat (errMessage, "SAMPLE OK");
                          break;
                        case 1:
                          strcat (errMessage, "NO LIQUID FOUND");
                          break;
                        case 2:
                          strcat (errMessage, "LIQUID LOST");
                          break;
                        case 3:
                          strcat (errMessage,  "USER STOPPED");
                          break;
                        case 4:
                          strcat (errMessage,  "POWER FAILED");
                          break;
                        case 5:
                          strcat (errMessage,  "PUMP JAMMED");
                          break;
                        case 6:
                          strcat (errMessage,   "DISTRIBUTOR JAMMED");
                          break;
                        case 8:
                          strcat (errMessage,  "PUMP LATCH OPEN");
                          break;  
                        case 9:
                          strcat (errMessage,  "SAMPLER SHUT OFF");
                          break;   
                        case 11:
                          strcat (errMessage,   "NO DISTRIBUTOR");
                          break; 
                        case 12:
                          strcat (errMessage,   "SAMPLE IN PROGRESS");
                          break; 
                        default: 
                          // if nothing else matches, do the default
                          // default is optional
                          strcat (errMessage, "ERROR: no result value");
                      }
                                          delay(100);
                    delay(200);
                    Serial.println(errMessage);
                    delay(100);
                }
                 else
                {
                  Serial.println("SQR=NULL");
                  return -1;
                }
                


                
              return 1; /// end position is <1. === otherwise out of order - multiple incomplete returns possible
}

int ISCOControl(String s)
{
        ISCOClear("a");
   //let's pretend last bottle was succesful 
  String STRvolume =  "1000";
  String STRbottle =  s;
  Serial1.write("BTL,");
  Serial1.print(STRbottle); // kludge. perhaps use sprintf http://liudr.wordpress.com/2012/01/16/sprintf/
  Serial1.write(",SVO,");
  Serial1.print(STRvolume); // also kludge
  Serial1.write('\r');
  
  
//  while (Serial1.available() !> 0){
//      //do nothing --- too risky
//      )
      
  for(int i=0;i<3;i++){
       Serial.println(i);
        call(); // get the serial response
        Serial1.flush();
        Serial1.write('\r');
        if (inSerial == '\0') {
            delay(14000);
        }
        else delay(200);
  }

}

///Ideally, I would have a function to update the time on the autosampler with the 
//internet time once per day or something but that isn't important.
