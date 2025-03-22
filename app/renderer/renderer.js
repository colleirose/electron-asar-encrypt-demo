// In addition to disabling DevTools through the window configuration, we add this script that will block opening DevTools unless it's removed.
// See https://www.npmjs.com/package/disable-devtool
import DisableDevtool from 'disable-devtool';

DisableDevtool(
    {
        // This is an MD55 hash of a password you choose
        // You can use the password to bypass the DevTools restriction after enabling DevTools in the window config
        // by navigating to index.html?tkName=(the plaintext password) in the Electron app
        // 
        // Currently, the password is "example"
        md5: "1a79a4d60de6718e8e5b326e338ae533",
        // This is the URL that the user will be redirected to if they try to open DevTools
        url: "about:blank",
        // Comment out this line if you want to disable the right click menu within the app
        disableMenu: false
    }
)

// This script runs in the renderer with index.html. Feel free to add more content to it.