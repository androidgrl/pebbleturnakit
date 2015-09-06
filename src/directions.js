var xhrRequest = function (url, type, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    callback(this.responseText);
  };
  xhr.open(type, url);
  xhr.send();
};

function locationSuccess(pos) {
    lat = String(pos.coords.latitude).replace(".","p");
    lon = String(pos.coords.longitude).replace(".","p");
//   lat = String("40.0174031").replace(".","p");
//   lon = String("-105.2833811").replace(".","p");
  var URL = ['https://turnakitmapper.herokuapp.com/direction/', lat, '&', lon].join("");
  
  // Send request to OpenWeatherMap
  xhrRequest(URL, 'GET', 
    function(response) {
      var response = JSON.parse(response);

      var direction = response.direction;
      var distance = response.distance;
      console.log(response);
      console.log(direction,distance);
      
      var dictionary = {
        "KEY_DIRECTION": direction,
        "KEY_DISTANCE": distance
      };

      // Send to Pebble
      Pebble.sendAppMessage(dictionary,
        function(e) {
          console.log("Weather info sent to Pebble successfully!");
        },
        function(e) {
          console.log("Error sending weather info to Pebble!");
        }
      );
    }      
  );
}

function locationError(err) {
  console.log("Error requesting location!");
}

function getDir() {
  navigator.geolocation.getCurrentPosition(
    locationSuccess,
    locationError,
    {timeout: 15000, maximumAge: 60000}
  );
}

setInterval(getDir, 7500);

// Listen for when the watchface is opened
Pebble.addEventListener('ready', 
  function(e) {
    console.log("PebbleKit JS ready!");

    // Get the initial weather
    getDir();
  }
);