/** API URL pointing to the backend server exposed via ngrok. */
//const apiUrl = "https://c0b1-2806-261-484-b18-f388-fad9-1f72-76b2.ngrok-free.app/"; 

/** Uncomment this line for production deployment with tunnel */
//const apiUrl = "https://k3lr72nn-3000.usw3.devtunnels.ms/"; 

/** Uncomment this line for local testing with the backend */
const apiUrl = "http://localhost:3000/";

/** 
 * History object to store up to the last 60 entries of sensor and actuator data
 * Key names correspond to various data points tracked by the system.
 */
const historyData = {
  ldr: [], /** Ambient Light */
  pir: [], /** Presence Detection */
  lvl: [], /** Water Level */
  tmp: [], /** Temperature */
  hum: [], /** Humidity */
  well: [], /** Well Water Level */
  lmp: [], /** Lamp Status */
  pmp: [], /** Pump Status */
  irr: [] /** Irrigation Status */
};

/** 
 * Fetches data from the backend server for a specified type (e.g., sensors or actuators).
 * Includes the "ngrok-skip-browser-warning" header to bypass ngrok's warning page.
 */
async function fetchData(type) {
  if (!window.selectedChipId) return null;
  const endpoint = `getLastData?chipId=${encodeURIComponent(window.selectedChipId)}&type=${type}`;
  const finalUrl = `${apiUrl}${endpoint}`;
  try {
    const response = await fetch(finalUrl, {
      headers: {
        "ngrok-skip-browser-warning": "true",
      },
    });
    if (!response.ok) throw new Error(`Failed to fetch ${type} data`);
    const data = await response.json();
    console.log(`Fetched ${type} data:`, data);
    return data;
  } catch (error) {
    console.error(`Error fetching ${type} data:`, error);
    return null;
  }
}

/** 
 * Returns the corresponding icon based on the data title and value provided.
 * Icons visually represent various data points, such as temperature or actuator status.
 */
function getIcon(title, value) {
  switch (title) {
    case "Temperature": 
      return '<i class="fas fa-thermometer-half"></i>';
    case "Humidity": 
      return '<i class="fas fa-tint"></i>';
    case "Cistern Level": 
      return '<i class="fas fa-water"></i>';
    case "Well Level": 
      return value === "FULL"
      ? '<i class="fas fa-water" style="color:#5bc0de;"></i>'
      : '<i class="fas fa-water" style="color:gray;"></i>';
    case "Ambient Light": 
      return value === "Dark" 
      ? '<i class="fas fa-moon"></i>' 
      : '<i class="fas fa-sun"></i>';
    case "Precense": 
      return value === "Detected" 
      ? '<i class="fas fa-walking"></i>' 
      : '<i class="fas fa-user-slash"></i>';
    case "Lamp": 
      return value === "ON" 
      ? '<i class="fas fa-lightbulb"></i>' 
      : '<i class="far fa-lightbulb"></i>';
    case "Pump": 
      return value === "ON"
      ? '<i class="fas fa-faucet" style="color:lightblue;"></i>'
      : '<i class="fas fa-faucet" style="color:gray;"></i>';
    case "Irrigation": 
      return value === "ON" 
        ? '<i class="fas fa-cloud-showers-water" style="color:#5bc0de;"></i>' 
        : '<i class="fas fa-cloud-showers-water" style="color:gray;"></i>';
    default: return '<i class="fas fa-microchip"></i>'; /** Default icon for unknown data types */
  }
}

/** 
 * Creates or updates a card element to display the latest data for a specific key.
 * Handles both the creation of new cards and the update of existing cards.
 */
function createOrUpdateCard(groupEl, title, unit, key, data) {
  const id = `card-${key}`;
  let card = document.getElementById(id);

  let displayTitle = title;
  let displayValue;
  let displayUnit = unit;

  displayValue = data[key] !== undefined
    ? key === "ldr" ? (data[key] === "1" ? "Dark" : "Light")
    : key === "pir" ? (data[key] === "0" ? "None" : "Detected")
    : key === "pmp" ? (data[key] === "0" ? "OFF" : "ON")
    : key === "lmp" ? (data[key] === "1" ? "ON" : "OFF")
    : key === "irr" ? (data[key] === "1" ? "ON" : "OFF")
    : key === "well" ? (data[key] === "0" ? "FULL" : "LOW")
    : data[key]
    : "N/A";

  /** Save the current data entry to history */
  if (data.timestamp) {
    historyData[key].push({ 
      timestamp: data.timestamp, 
      value: displayValue, 
      unit: displayUnit 
    });

    /** Keep only the last 60 entries in history */
    if (historyData[key].length > 60) {
      historyData[key].shift(); 
    }
  }

  if (!card) {
    /** Create a new card if one doesn't exist */
    card = document.createElement("div");
    card.className = "card";
    card.id = id;
    card.onclick = () => flipCard(card, key);
    card.innerHTML = `
      <div class="card-inner">
        <div class="card-front">
          ${getIcon(displayTitle, displayValue)}
          <h3>${displayTitle}</h3>
          <p><span class="value">${displayValue}</span> <span>${displayUnit}</span></p>
        </div>
        <div class="card-back">
          <h4>History</h4>
          <div class="history-list" style="margin-top:10px;"></div>
        </div>
      </div>
    `;
    groupEl.appendChild(card);
  } else {
    /** Update the value in the existing card */
    const valueEl = card.querySelector(".value");
    const oldRawValue = card.dataset.rawValue;
    card.dataset.rawValue = data[key];

    // Update title and icon if needed
    card.querySelector("h3").textContent = displayTitle;
    card.querySelector("i").outerHTML = getIcon(displayTitle, displayValue);

    /* For comparison, use the raw data value (not the formatted/displayed value) */  
    if (oldRawValue !== undefined && oldRawValue !== String(data[key])) {
      valueEl.textContent = displayValue;
      card.classList.add("flash"); /** Flash the card to indicate an update */
      setTimeout(() => card.classList.remove("flash"), 800);
    } else if (oldRawValue === undefined) {
      /** On first render, just set the value */ 
      valueEl.textContent = displayValue;
    }
  }
}

/** 
 * Flips the card to display its history and ensures duplicate timestamps are removed.
 */
function flipCard(card, key) {
  card.classList.toggle("flipped");

  if (card.classList.contains("flipped")) {
    const historyList = card.querySelector(".history-list");
    historyList.innerHTML = "";

    const type = ["lmp", "pmp", "irr"].includes(key) ? "actuators" : "sensors";
    if (!window.selectedChipId) {
      historyList.innerHTML = "<div class='history-item'>No device selected.</div>";
      return;
    }
    const endpoint = `getHistoryData?chipId=${encodeURIComponent(window.selectedChipId)}&type=${type}&key=${key}`;
    const historyApiUrl = `${apiUrl}${endpoint}`;

    fetch(historyApiUrl)
      .then((response) => {
        if (!response.ok) throw new Error("Failed to fetch history data");
        return response.json();
      })
      .then((history) => {
        history.forEach((item) => {
          const value = item[key] !== undefined
            ? key === "ldr" ? (item[key] === "1" ? "Dark" : "Light")
            : key === "pir" ? (item[key] === "0" ? "None" : "Detected")
            : key === "pmp" ? (item[key] === "0" ? "OFF" : "ON")
            : key === "lmp" ? (item[key] === "1" ? "ON" : "OFF")
            : key === "irr" ? (item[key] === "1" ? "ON" : "OFF")
            : key === "well" ? (item[key] === "0" ? "FULL" : "LOW")
            : item[key]
            : "N/A";

          const unit = key === "lvl" ? "%" : key === "tmp" ? "°C" : key === "hum" ? "%" : "";

          const date = new Date(item.timestamp);
          const formattedDate = date.toLocaleString();
          const div = document.createElement("div");
          div.className = "history-item";
          div.textContent = `${formattedDate}: ${value} ${unit}`;
          historyList.appendChild(div);
        });
      })
      .catch((error) => {
        console.error("Error fetching history data:", error);
        const errorDiv = document.createElement("div");
        errorDiv.className = "history-item";
        errorDiv.textContent = "Error loading history.";
        historyList.appendChild(errorDiv);
      });
  }
}

/** 
 * Fetches and updates the dashboard with the latest sensor and actuator data.
 */
async function updateDashboard() {
  if (!window.selectedChipId) {
    document.getElementById("status-text").textContent = "Please register and select a device.";
    return;
  }

  const g1 = document.getElementById("group1"); // Group for cards like Ambient Light, Presence, and Lamp
  const g2 = document.getElementById("group2"); // Group for cards like Water Level and Pump
  const g3 = document.getElementById("group3"); // Group for cards like Temperature and Humidity
  const status = document.getElementById("status"); // Status display area

  let updateTime = null;

  /** Fetch and update sensor data */
  const sensorData = await fetchData("sensors");
  if (sensorData) {
    console.log("Updating sensor data:", sensorData);
    createOrUpdateCard(g1, "Ambient Light", "", "ldr", sensorData);
    createOrUpdateCard(g1, "Precense", "", "pir", sensorData);
    createOrUpdateCard(g2, "Cistern Level", "%", "lvl", sensorData);
    createOrUpdateCard(g2, "Well Level", "", "well", sensorData);
    createOrUpdateCard(g3, "Temperature", "°C", "tmp", sensorData);
    createOrUpdateCard(g3, "Humidity", "%", "hum", sensorData);

    /** Save the timestamp if it exists */
    updateTime = sensorData.timestamp;
  }

  /** Fetch and update actuator data */
  const actuatorData = await fetchData("actuators");
  if (actuatorData) {
    console.log("Updating actuator data:", actuatorData);
    createOrUpdateCard(g1, "Lamp", "", "lmp", actuatorData);
    createOrUpdateCard(g2, "Pump", "", "pmp", actuatorData);
    createOrUpdateCard(g3, "Irrigation", "", "irr", actuatorData);
  }

  /** Update the status text to reflect the latest data */
  const statusIcon = document.getElementById("status-icon");
  const statusText = document.getElementById("status-text");

  if (updateTime) {
    const date = new Date(updateTime);
    const now = new Date();
    const elapsedMinutes = (now - date) / (1000 * 60); /** Calculate elapsed time in minutes */

    if (elapsedMinutes > 3) {
      /** If more than 3 minutes have passed since the last update */
      statusText.textContent = "Device connection error: Displaying last received data.";
      status.style.color = "#ff8888"; // Red for error
      statusIcon.className = "fas fa-exclamation-triangle";
      statusIcon.style.color = "#ff5555"; /** Red for error */
    } else {
      /** If the last update was within 3 minutes */
      statusText.textContent = `Last Update: ${date.toLocaleString()}`;
      status.style.color = "#aaa"; // Neutral gray
      statusIcon.className = "fas fa-check-circle";
      statusIcon.style.color = "#90ee90"; /** Green for success */
    }
  } else {
    /** If no update time is available */
    statusText.textContent = "No data found for this device. Please check if the device is powered on and connected to the internet.";
    status.style.color = "#ff8888"; // Red for error
    statusIcon.className = "fas fa-exclamation-triangle";
    statusIcon.style.color = "#ff5555"; /** Red for error */
  }
}

/** 
 * Fetch current settings from the backend and populate the settings menu
 */
async function loadSettings() {
  if (!window.selectedChipId) return;
  const endpoint = `getSettings?chipId=${encodeURIComponent(window.selectedChipId)}`;
  const settingsApiUrl = `${apiUrl}${endpoint}`;
  try {
    const response = await fetch(settingsApiUrl);
    if (response.status === 404) {
      alert("No data found for this device. Please check if the device is powered on and connected to the internet.");
      document.getElementById("min-level").value = "";
      document.getElementById("max-level").value = "";
      document.getElementById("low-humidity").value = "";
      document.getElementById("hot-temperature").value = "";
      return;
    }
    if (!response.ok) throw new Error("Failed to fetch settings");
    const settings = await response.json();
    document.getElementById("min-level").value = settings.minLevel !== "NA" ? settings.minLevel : "";
    document.getElementById("max-level").value = settings.maxLevel !== "NA" ? settings.maxLevel : "";
    document.getElementById("low-humidity").value = settings.lowHumidity !== "NA" ? settings.lowHumidity : "";
    document.getElementById("hot-temperature").value = settings.hotTemperature !== "NA" ? settings.hotTemperature : "";
  } catch (error) {
    console.error("Error loading settings:", error);
    alert("Failed to load current settings. Please try again later.");
  }
}

/** Call loadSettings when the page loads */
window.addEventListener("load", async () => {
  if (typeof loadRegisteredDevices === "function") {
    await loadRegisteredDevices();
  }
  loadSettings();
  updateDashboard();
  setInterval(updateDashboard, 5000);
});

// Toggle settings menu visibility
document.querySelectorAll('.settings-icon').forEach(icon => {
  icon.addEventListener('click', (event) => {
    const group = event.target.closest('.group');
    const settingsMenu = group.querySelector('.settings-menu');
    settingsMenu.style.display = settingsMenu.style.display === 'none' ? 'block' : 'none';
  });
});

/** Toggle settings menu visibility */
const settingsIcon = document.getElementById("settings-icon");
const settingsMenu = document.getElementById("settings-menu");

settingsIcon.addEventListener("click", () => {
  if (settingsMenu) {
    const isHidden = settingsMenu.style.display === "none";
    settingsMenu.style.display = isHidden ? "block" : "none";

    if (isHidden) {
      loadSettings(); /** Reload settings when the menu is opened */
    }
  } else {
    console.error("Settings menu element not found.");
  }
});

/** 
 * Handle form submission for saving settings.
 */
const settingsForm = document.getElementById("settings-form");

settingsForm.addEventListener("submit", (event) => {
  event.preventDefault();
  if (!window.selectedChipId) {
    alert("Please select a device first.");
    return;
  }

  /** Get input values */
  const minLevelInput = document.getElementById("min-level");
  const maxLevelInput = document.getElementById("max-level");
  const lowHumidityInput = document.getElementById("low-humidity");
  const hotTemperatureInput = document.getElementById("hot-temperature");

  /** Prepare the userSettings object */
  const userSettings = {
    minLevel: minLevelInput.value ? parseFloat(minLevelInput.value) : "NA",
    maxLevel: maxLevelInput.value ? parseFloat(maxLevelInput.value) : "NA",
    lowHumidity: lowHumidityInput.value ? parseFloat(lowHumidityInput.value) : "NA",
    hotTemperature: hotTemperatureInput.value ? parseFloat(hotTemperatureInput.value) : "NA",
  };

  /** Validation logic */
  if ((userSettings.minLevel !== "NA" && userSettings.maxLevel === "NA") ||
      (userSettings.maxLevel !== "NA" && userSettings.minLevel === "NA")) {
    alert("If you change Min Level, you must also change Max Level, and vice versa.");
    return;
  }

  if ((userSettings.lowHumidity !== "NA" && userSettings.hotTemperature === "NA") ||
      (userSettings.hotTemperature !== "NA" && userSettings.lowHumidity === "NA")) {
    alert("If you change Low Humidity, you must also change Hot Temperature, and vice versa.");
    return;
  }

  /** Define the endpoint for saving settings */
  const endpoint = "saveSettings";
  const settingsApiUrl = `${apiUrl}${endpoint}`;

  /** Send settings to the server */
  fetch(settingsApiUrl, {
    method: "POST",
    headers: {
      "Content-Type": "application/json",
    },
    body: JSON.stringify({ chipId: window.selectedChipId, userSettings }),
  })
    .then((response) => {
      if (!response.ok) throw new Error("Failed to save settings");
      alert("Settings saved successfully!");
    })
    .catch((error) => {
      console.error("Error saving settings:", error);
      alert("Failed to save settings.");
    });
});