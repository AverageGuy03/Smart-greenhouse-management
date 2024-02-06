#include <DHT.h>
#include <WiFi.h>
#include <ESP_Mail_Client.h>
#include <ThingSpeak.h>

#define relayPin 5  //  Define Pin of the Relay

// Replace with your network credentials
const char *ssid = "Subdid";
const char *password = "qwertyuiop";
WiFiClient client;

// The smtp host name e.g. smtp.gmail.com for GMail or smtp.office365.com for Outlook or smtp.mail.yahoo.com
#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465

/* The sign in credentials */
#define AUTHOR_EMAIL "ssanjay3184@gmail.com"
#define AUTHOR_PASSWORD "gdmn tsne hywi zvul"

/* Recipient's email*/
#define RECIPIENT_EMAIL "1nonly.user@gmail.com"

/* Declare the global used SMTPSession object for SMTP transport */
SMTPSession smtp;

// Replace with your ThingSpeak Channel ID and API Key
const unsigned long channelId = 2415163;
const char *apiKey = "02XPAJZZ70BPYVCO";

// DHT sensor setup
#define DHTPIN 15   // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11   // DHT 11
DHT dht(DHTPIN, DHTTYPE);

int soilMoisturePercentage,sensor_analog;
const int soilmoisture_pin = 34;  /* Soil moisture sensor O/P pin */

void setup(){

  pinMode(relayPin,OUTPUT);         // Setting the Pin to output signal
  
  // Start serial communication
  Serial.begin(115200);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Initialize DHT sensor
  dht.begin();

  /*  Set the network reconnection option */
  MailClient.networkReconnect(true);

  smtp.debug(1);

  // Initialize ThingSpeak
  ThingSpeak.begin(client);

}

/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status){
  /* Print the current status */
  Serial.println(status.info());

  /* Print the sending result */
  if (status.success()){

    for (size_t i = 0; i < smtp.sendingResult.size(); i++)
    {
      /* Get the result item */
      SMTP_Result result = smtp.sendingResult.getItem(i);
      ESP_MAIL_PRINTF("Status: %s\n", result.completed ? "success" : "failed");
      ESP_MAIL_PRINTF("Date/Time: %s\n", MailClient.Time.getDateTimeString(result.timestamp, "%B %d, %Y %H:%M:%S").c_str());
      ESP_MAIL_PRINTF("Recipient: %s\n", result.recipients.c_str());
      ESP_MAIL_PRINTF("Subject: %s\n", result.subject.c_str());
    }
    Serial.println("----------------\n");

    // To clear sending result as the memory usage will grow up.
    smtp.sendingResult.clear();
  }
}

void loop(){

  // Read temperature and humidity from DHT sensor
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  sensor_analog = analogRead(soilmoisture_pin);
  soilMoisturePercentage = ( 100 - ( (sensor_analog/4095.00) * 100 ) );

  // Check if DHT had any reads failed and exit early
  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  if (temperature>=27) {
    // Writing value "ON" to the pin
    digitalWrite(relayPin,LOW);
  } 
  else{
    digitalWrite(relayPin,HIGH);
  }

  // Print sensor values to Serial Monitor
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print("°C, Humidity: ");
  Serial.print(humidity);
  Serial.print("%, soil Moisture: ");
  Serial.print(soilMoisturePercentage);
  Serial.println("%");
  Serial.println("");
  
  if (soilMoisturePercentage<55){
    Serial.println("low soilmoisture");
    /* Declare the message class */
    SMTP_Message message;

    /* Set the message headers */
    message.sender.name = F("ESP");
    message.sender.email = AUTHOR_EMAIL;
    message.subject = F("Smart greenhouse management");
    message.addRecipient(F("Customer"), RECIPIENT_EMAIL);
    
    //Send raw text message
    String textMsg = "Time to water the plants !!!";
    message.text.content = textMsg.c_str();
    message.text.charSet = "us-ascii";
    message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;
    
    message.priority = esp_mail_smtp_priority::esp_mail_smtp_priority_low;
    message.response.notify = esp_mail_smtp_notify_success | esp_mail_smtp_notify_failure | esp_mail_smtp_notify_delay;

    /* Declare the Session_Config for user defined session credentials */
    Session_Config config;

    /* Set the session config */
    config.server.host_name = SMTP_HOST;
    config.server.port = SMTP_PORT;
    config.login.email = AUTHOR_EMAIL;
    config.login.password = AUTHOR_PASSWORD;
    config.login.user_domain = "";

    config.time.ntp_server = F("pool.ntp.org,time.nist.gov");
    config.time.gmt_offset = 3;
    config.time.day_light_offset = 0;

    /* Connect to the server */
    if (!smtp.connect(&config)){
      ESP_MAIL_PRINTF("Connection error, Status Code: %d, Error Code: %d, Reason: %s", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());
      return;
    }

    if (!smtp.isLoggedIn()){
      Serial.println("\nNot yet logged in.");
    }
    else{
      if (smtp.isAuthenticated())
        Serial.println("\nSuccessfully logged in.");
      else
        Serial.println("\nConnected with no Auth.");
    }

    /* Start sending Email and close the session */
    if (!MailClient.sendMail(&smtp, &message)){
      ESP_MAIL_PRINTF("Error, Status Code: %d, Error Code: %d, Reason: %s", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());
    }
  }

  // set the fields with the values
  ThingSpeak.setField(1, temperature);
  ThingSpeak.setField(2, humidity);
  ThingSpeak.setField(3, soilMoisturePercentage);
  // Update ThingSpeak channel with sensors data
  ThingSpeak.writeFields(channelId, apiKey);

  // Wait for 10 seconds before updating again
  delay(10000);
}