String sendHTML(String output26State, String output27State) {
    String outString = "";
    
    // Display the HTML web page
    outString.concat("<!DOCTYPE html><html>");
    outString.concat("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
    outString.concat("<link rel=\"icon\" href=\"data:,\">");
    // CSS to style the on/off buttons 
    // Feel free to change the background-color and font-size attributes to fit your preferences
    outString.concat("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
    outString.concat(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
    outString.concat("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
    outString.concat(".button2 {background-color: #555555;}</style></head>");
    
    // Web Page Heading
    outString.concat("<body><h1>Clock Setup</h1>");
    
    // Display current state, and ON/OFF buttons for GPIO 26  
    outString.concat("<p>GPIO 26 - State " + output26State + "</p>");
    // If the output26State is off, it displays the ON button       
    if (output26State=="off") {
      outString.concat("<p><a href=\"/26/on\"><button class=\"button\">ON</button></a></p>");
    } else {
      outString.concat("<p><a href=\"/26/off\"><button class=\"button button2\">OFF</button></a></p>");
    } 
       
    // Display current state, and ON/OFF buttons for GPIO 27  
    outString.concat("<p>GPIO 27 - State " + output27State + "</p>");
    // If the output27State is off, it displays the ON button       
    if (output27State=="off") {
      outString.concat("<p><a href=\"/27/on\"><button class=\"button\">ON</button></a></p>");
    } else {
      outString.concat("<p><a href=\"/27/off\"><button class=\"button button2\">OFF</button></a></p>");
    }
    outString.concat("</body></html>");
    
    // The HTTP response ends with another blank line
    return outString;
}
