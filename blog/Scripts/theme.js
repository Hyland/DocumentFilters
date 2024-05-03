// Wait for the DOM to be fully loaded before running the script
document.addEventListener('DOMContentLoaded', () => {
    // Get references to the dropdown, body, content, and inner content elements
    const body = document.body;
    const themeSelect = document.getElementById("theme-select");
    const content = document.querySelector('.content');
    const innerContent = document.querySelector('.inner-content');

    // Function to apply the selected theme
    const applyTheme = (theme) => {
        // Remove any existing theme classes
        body?.classList.remove("light-theme-body", "dark-theme-body");
        content?.classList.remove("light-theme", "dark-theme");
        innerContent?.classList.remove("light-theme", "dark-theme");
        themeSelect?.classList.remove("light-theme", "dark-theme");

        // Add the new theme class based on the selected value
        body?.classList.add(theme + "-theme-body");
        content?.classList.add(theme + "-theme");
        innerContent?.classList.add(theme + "-theme");
        themeSelect?.classList.add(theme + "-theme");
    };

    // Check if a theme has been saved in local storage
    const savedTheme = localStorage.getItem('theme');
    if (savedTheme) {
        // If a saved theme exists, apply it to the page
        applyTheme(savedTheme);
        // Set the dropdown's value to match the saved theme
        themeSelect.value = savedTheme;
    }
    else {
        // If there is no saved theme, apply the dark theme by default
        applyTheme('dark');
        themeSelect.value = 'dark';
        localStorage.setItem('theme', 'dark');
    }

    // Listen for changes in the dropdown selection
    themeSelect.addEventListener("change", () => {
        // Apply the selected theme
        applyTheme(themeSelect.value);
        // Save the selected theme to local storage
        localStorage.setItem('theme', themeSelect.value);
    });
});