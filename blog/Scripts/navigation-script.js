// Function to update the current URL hash and smoothly scroll to the element with the given ID
function updateHash(id) {
  window.history.pushState({}, '', '#' + id); // Adds a new entry to the browser's session history stack
  const element = document.getElementById(id); // Retrieves the element with the specified ID
  if (element) {
    element.scrollIntoView({ behavior: 'smooth' }); // Scrolls the element into view with a smooth animation
  }
}

// Function to handle navigation based on the URL hash
function handleHashNavigation() {
  if (window.location.hash) { // Checks if there is a hash in the URL
    setTimeout(function() { // Delays the execution to ensure the page is fully loaded
      updateHash(window.location.hash.substring(1)); // Calls updateHash with the hash value, excluding the '#' character
    }, 100); // Sets a 100ms delay before executing
  }
}

// Event listener for when the DOM content is fully loaded
document.addEventListener('DOMContentLoaded', handleHashNavigation); // Calls handleHashNavigation once the DOM is ready

// Event listener for when the browser's history entry changes
window.addEventListener('popstate', handleHashNavigation); // Calls handleHashNavigation when the active history entry changes

// Function to initialize any necessary JavaScript when the window loads
function initialize() {
  Gifffer(); // Initializes Gifffer to control GIF animations
  
  handleHashNavigation(); // Calls the function to handle hash navigation on page load
}

// Assigns the initialize function to be called when the window finishes loading
window.onload = initialize;
