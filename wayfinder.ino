
#include <WiFiS3.h>

#define AIN1 2
#define AIN2 3
#define BIN1 4
#define PWMA 5
#define PWMB 6
#define BIN2 7
#define STBY 8

char ssid[] = "WAYFINDER_R4";
char password[] = "wayfinder123";

WiFiServer server(80);

// Safety timeout in milliseconds
const unsigned long TIMEOUT = 700;
unsigned long lastCommand = 0;
bool moving = false;

// MOTOR FUNCTIONS
void moveForward() {
  digitalWrite(STBY, HIGH);

  // Verified forward directions
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, HIGH);
  digitalWrite(BIN1, HIGH);
  digitalWrite(BIN2, LOW);

  digitalWrite(PWMA, HIGH);
  digitalWrite(PWMB, HIGH);
}

void moveBackward() {
  digitalWrite(STBY, HIGH);

  digitalWrite(AIN1, HIGH);
  digitalWrite(AIN2, LOW);
  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, HIGH);

  digitalWrite(PWMA, HIGH);
  digitalWrite(PWMB, HIGH);
}

void turnLeft() {
  digitalWrite(STBY, HIGH);

  // Left wheel backward, right forward
  digitalWrite(AIN1, HIGH);
  digitalWrite(AIN2, LOW);
  digitalWrite(BIN1, HIGH);
  digitalWrite(BIN2, LOW);

  digitalWrite(PWMA, HIGH);
  digitalWrite(PWMB, HIGH);
}

void turnRight() {
  digitalWrite(STBY, HIGH);

  // Left wheel forward, right backward
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, HIGH);
  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, HIGH);

  digitalWrite(PWMA, HIGH);
  digitalWrite(PWMB, HIGH);
}

void stopMotors() {
  digitalWrite(PWMA, LOW);
  digitalWrite(PWMB, LOW);

  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, LOW);
  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, LOW);

  moving = false;
}

void setup() {
  Serial.begin(9600);

  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);
  pinMode(PWMA, OUTPUT);
  pinMode(PWMB, OUTPUT);
  pinMode(STBY, OUTPUT);

  digitalWrite(STBY, HIGH);
  stopMotors();

  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("WiFi module missing");
    while (true);
  }

  Serial.println("Starting Wayfinder...");

  if (WiFi.beginAP(ssid, password) != WL_AP_LISTENING) {
    Serial.println("WiFi AP failed");
    while (true);
  }

  delay(1000);
  server.begin();

  Serial.print("Network: ");
  Serial.println(ssid);
  Serial.print("Address: http://");
  Serial.println(WiFi.localIP());
}

void loop() {
  // Stop if commands stop arriving
  if (moving && millis() - lastCommand > TIMEOUT) {
    stopMotors();
  }

  WiFiClient client = server.available();

  if (client) {
    client.setTimeout(100);

    String request = client.readStringUntil('\n');

    // Only accept motor commands
    // from the /cmd endpoint
    if (request.startsWith("GET /cmd?d=")) {
      char direction = request.charAt(11);

      if (direction == 'F') {
        moveForward();
        moving = true;
      }
      else if (direction == 'B') {
        moveBackward();
        moving = true;
      }
      else if (direction == 'L') {
        turnLeft();
        moving = true;
      }
      else if (direction == 'R') {
        turnRight();
        moving = true;
      }
      else {
        stopMotors();
      }

      lastCommand = millis();

      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/plain");
      client.println("Connection: close");
      client.println();
      client.println("OK");
    }
    else {
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/html");
      client.println("Connection: close");
      client.println();

      client.print(R"HTML(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport"
content="width=device-width, initial-scale=1">
<style>
body {
  background: #111827;
  color: white;
  font-family: Arial;
  text-align: center;
  padding-top: 25px;
}
button {
  width: 95px;
  height: 80px;
  margin: 5px;
  border: none;
  border-radius: 14px;
  background: #2563eb;
  color: white;
  font-size: 28px;
  touch-action: none;
  user-select: none;
}
.stop { background: #dc2626; }
button:active { background: #16a34a; }
</style>
</head>
<body>
<h1>WAYFINDER</h1>
<p>Manual Robot Controller</p>

<div>
<button data-dir="F">▲</button>
</div>
<div>
<button data-dir="L">◀</button>
<button class="stop" data-dir="S">■</button>
<button data-dir="R">▶</button>
</div>
<div>
<button data-dir="B">▼</button>
</div>

<p>Hold a direction to move.<br>
Release to stop.</p>

<script>
let timer = null;
let active = "S";

function send(d) {
  fetch("/cmd?d=" + d, {cache:"no-store"})
    .catch(() => {});
}

function stop() {
  clearInterval(timer);
  timer = null;
  active = "S";
  send("S");
}

document.querySelectorAll("button").forEach(b => {
  b.addEventListener("pointerdown", e => {
    e.preventDefault();
    if (b.dataset.dir === "S") {
      stop();
      return;
    }
    stop();
    active = b.dataset.dir;
    send(active);
    timer = setInterval(() => send(active), 200);
  });

  b.addEventListener("pointerup", stop);
  b.addEventListener("pointercancel", stop);
  b.addEventListener("lostpointercapture", stop);
  b.addEventListener("contextmenu", e => e.preventDefault());
});

window.addEventListener("blur", stop);
document.addEventListener("visibilitychange", () => {
  if (document.hidden) stop();
});
</script>
</body>
</html>
)HTML");
    }

    client.flush();
    client.stop();
  }
}
