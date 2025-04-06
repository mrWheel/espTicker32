//----- espTicker32.js -----
// Array to store input field values
let inputFields = [];

// Function to check if the page is ready for input fields
function isPageReadyForInputFields() {
  // Check if the inputTableBody element exists
  const tableBody = document.getElementById('inputTableBody');
  return !!tableBody;
} // isPageReadyForInputFields()


// Function to initialize the input fields from JSON
function initializeInputFields(jsonString) 
{
  console.log('initializeInputFields called with:', jsonString);
  try {
    inputFields = JSON.parse(jsonString) || [];
    renderInputFields();
  } catch (e) {
    console.error('Error parsing JSON:', e);
    inputFields = [];
  }
}

// Function to render the input fields in the table
function renderInputFields() 
{
  console.log('renderInputFields called');
  
  // Check if the page is ready
  if (!isPageReadyForInputFields()) {
    console.error('Input table body not found in DOM, page not ready yet');
    return;
  }
  
  const tableBody = document.getElementById('inputTableBody');
  tableBody.innerHTML = '';
  
  inputFields.forEach((value, index) => {
    const row = document.createElement('tr');
    const cell = document.createElement('td');
    cell.style.padding = '8px';
    
    const input = document.createElement('input');
    input.type = 'text';
    input.value = value;
    input.style.width = '100%';
    input.style.maxWidth = '80ch';
    input.maxLength = 80;
    input.dataset.index = index;
    input.addEventListener('input', (e) => {
      inputFields[e.target.dataset.index] = e.target.value;
    });
    
    cell.appendChild(input);
    row.appendChild(cell);
    tableBody.appendChild(row);
  });

} // renderInputFields()

// Function to add a new input field
function addInputField() 
{
  console.log('addInputField called');
  inputFields.push('');
  renderInputFields();
}

// Function to save input fields via WebSocket
function saveInputFields() 
{
  console.log('saveInputFields called');
  if (window.ws && window.ws.readyState === WebSocket.OPEN) 
  {
    window.ws.send(JSON.stringify({
      type: 'process',
      processType: 'saveInputFields',
      inputValues: { 'inputFieldsData': JSON.stringify(inputFields) }
    }));
  } else {
    console.error('WebSocket is not connected');
  }
}

// This function is called by SPAmanager to check if the script is loaded
function isEspTicker32Loaded() 
{
  console.log("isEspTicker32Loaded called");
  
  // Check if we already have a WebSocket connection
  if (!window.ws || window.ws.readyState !== WebSocket.OPEN) {
    console.log("WebSocket not available or not open, checking global ws variable");
    
    // Try to use the global ws variable from SPAmanager.js
    if (typeof ws !== 'undefined' && ws.readyState === WebSocket.OPEN) {
      console.log("Using global ws variable from SPAmanager");
      window.ws = ws;
    } else {
      console.log("WebSocket not available yet, waiting...");
      setTimeout(isEspTicker32Loaded, 100);
      return true;
    }
  }
  
  console.log("WebSocket is available, setting up message handler");
  
  // Set up WebSocket message handler
  window.ws.addEventListener('message', (event) => {
    try {
      console.log("WebSocket message received:", event.data.substring(0, 50) + "...");
      
      // Try to parse the message as JSON
      const data = JSON.parse(event.data);
      
      // Check if this is our custom inputFieldsData message
      if (data.type === 'custom' && data.action === 'inputFieldsData') {
        console.log('Received input fields data');
        
        // Initialize with the data
        initializeInputFields(data.data);
      }
      // Also check for direct JSON arrays for backward compatibility
      else if (event.data.startsWith('[') && event.data.endsWith(']')) {
        console.log('Received direct input fields data');
        
        // Try to initialize immediately
        initializeInputFields(event.data);
      }
    } catch (e) {
      console.error('Error handling WebSocket message:', e);
      
      // If parsing as JSON fails, check if it's a direct JSON array
      if (event.data.startsWith('[') && event.data.endsWith(']')) {
        console.log('Received direct input fields data');
        
        // Try to initialize immediately
        initializeInputFields(event.data);
      }
    }
  });
  
  // Check if the page is ready for input fields
  if (isPageReadyForInputFields()) {
    // Request input fields data from the server
    console.log("Page is ready, requesting input fields data from server");
    requestInputFields();
  } else {
    // Page is not ready yet, wait and check again
    console.log("Page is not ready yet, waiting...");
    
    // Use a limited number of retries instead of an infinite interval
    let retryCount = 0;
    const maxRetries = 10;
    const checkInterval = setInterval(() => {
      retryCount++;
      if (isPageReadyForInputFields()) {
        console.log("Page is now ready, requesting input fields data from server");
        requestInputFields();
        clearInterval(checkInterval);
      } else if (retryCount >= maxRetries) {
        console.error("Max retries reached, page still not ready");
        clearInterval(checkInterval);
      } else {
        console.log(`Page still not ready, retry ${retryCount}/${maxRetries}`);
      }
    }, 500);
  }
  
  return true;
} // isEspTicker32Loaded()


// Function to request input fields data from the server
function requestInputFields() {
  console.log("Requesting input fields data from server");
  window.ws.send(JSON.stringify({
    type: 'requestInputFields'
  }));
} // requestInputFields()

// Log that the script has loaded
console.log("espTicker32.js has loaded");
