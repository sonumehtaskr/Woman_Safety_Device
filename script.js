var map = L.map('map').setView([30.858734, 75.861631], 15); // Set initial view with zoom level

// Add a tile layer with satellite imagery (HOT imagery from OpenStreetMap)
L.tileLayer('https://{s}.tile.openstreetmap.fr/hot/{z}/{x}/{y}.png', {
    attribution: '© <a href="https://www.openstreetmap.org/">OpenStreetMap</a> contributors, Imagery © <a href="https://www.hotosm.org/">Humanitarian OpenStreetMap Team</a>',
    maxZoom: 19 // Adjust max zoom level if needed
}).addTo(map);

// Function to fetch and update markers
function fetchAndUpdateMarkers() {
    var channelID = '2337349';
    var url = 'https://api.thingspeak.com/channels/' + channelID + '/feeds.json?results=50';
    fetch(url)
        .then(response => response.json())
        .then(data => {
            // Clear existing markers
            map.eachLayer(layer => {
                if (layer instanceof L.Marker) {
                    map.removeLayer(layer);
                }
            });

            // Access the 'feeds' array from the response data
            var feeds = data.feeds;
            // Loop through the feeds and create colored and numbered markers based on field3 value
            feeds.forEach((feed, index) => {
                var latitude = parseFloat(feed.field1);
                var longitude = parseFloat(feed.field2);
                var field3 = parseFloat(feed.field3);
                var createdAt = new Date(feed.created_at).toLocaleString(); // Convert timestamp to a readable format

                // Set marker color based on field3 value
                var markerColor = field3 === 1 ? 'red' : 'green';

                // Create a numbered and colored marker
                var marker = L.marker([latitude, longitude], {
                    icon: coloredNumberedIcon(index + 1, markerColor)
                }).addTo(map);
                // Bind popup with timestamp to marker
                marker.bindPopup(createdAt);
            });
        })
        .catch(error => {
            console.error('Error fetching JSON:', error);
        });
}

// Function to create colored and numbered icon
function coloredNumberedIcon(number, color) {
    return L.divIcon({
        className: 'colored-number-icon',
        html: '<div style="font-size: 14px; color: white; background-color: ' + color + '; border-radius: 50%; width: 24px; height: 24px; text-align: center; line-height: 24px;">' + number + '</div>'
    });
}

// Fetch and update markers initially
fetchAndUpdateMarkers();

// Refresh data every 30 seconds
setInterval(fetchAndUpdateMarkers, 30000);