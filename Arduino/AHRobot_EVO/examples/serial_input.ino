const byte numChars = 32;
char receivedChars[numChars]; // an array to store the received data

boolean newData = false;

void setup() {
    Serial.begin(115200);
    Serial.println("<Arduino is ready>");
    Serial.println();
    Serial.print("> ");
}

void loop() {
    receiveSerialData();
    processSerialCommand();
}

void receiveSerialData() {
    // https://forum.arduino.cc/index.php?topic=288234.0
    static byte ndx = 0;
    char endMarker = '\r';
    char rc;

    while (Serial.available() > 0 && newData == false) {
        rc = Serial.read();

        Serial.print(rc); // echo receieved data

        if (rc != endMarker) {
            receivedChars[ndx] = rc;
            ndx++;
            if (ndx >= numChars) {
                ndx = numChars - 1;
            }
        }
        else {
            receivedChars[ndx] = '\0'; // terminate the string
            ndx = 0;
            newData = true;
        }
    }
}

void processSerialCommand() {
    if (newData == true) {
        String command(receivedChars);
        command.trim();
        Serial.print("Processing: ");
        Serial.print(command);
        Serial.println();

        if (command.startsWith("G0")) {
            int x = getValue(command, "X", " ");
            int y = getValue(command, "Y", " ");
            Serial.print("Moving to position [");
            Serial.print(x);Serial.print(",");Serial.print(y);
            Serial.println("]");
        }
        else if (command.startsWith("X")) { 
            Serial.println("STOPPING!");
        }
        else {
            Serial.println("Command not supported");
        } 

        Serial.println("Done!");
        Serial.println();
        Serial.print("> ");
        newData = false;
    }
}

int getValue(String command, String argument, String delimiter) {
    
    int argIndex = command.indexOf(argument);
    int delimiterIndex = command.indexOf(delimiter, argIndex);
    int argLen = argument.length();
    
    return command.substring(argIndex + argLen, delimiterIndex).toInt();

}