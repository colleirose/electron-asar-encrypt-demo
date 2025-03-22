require("./asar.js");
const WindowManager = require("./window.js");
const { app, ipcMain } = require("electron");
const assert = require("assert");

const sanityTest = require("../node_modules_asar/modules-sanity-test");

if (!process.env.ELECTRON_RUN_AS_NODE) {
    console.log(`${process.type}: ${sanityTest}`);
} else {
    if (sanityTest !== "sanity test succeeded") {
        throw new Error(
            "node_modules.asar has not been loaded or has been loaded improperly"
        );
    }
}

if (!process.env.ELECTRON_RUN_AS_NODE) {
    app.on("activate", () => {
        WindowManager.createMainWindow();
    });
}

function mustNotExportKey(key) {
    ipcMain.on("check", (e, arr) => {
        if (arr.length !== key.length) {
            e.returnValue = {
                err: null,
                data: false
            };
            return;
        }

        for (let i = 0; i < key.length; i++) {
            try {
                if (key[i] !== arr[i]) {
                    e.returnValue = {
                        err: null,
                        data: false
                    };
                    return;
                }
            } catch (e) {
                e.returnValue = {
                    err: e.message,
                    data: false
                };
                return;
            }
        }

        e.returnValue = {
            err: null,
            data: true
        };
    });
}

function main() {
    WindowManager.createMainWindow();
    // WindowManager.getInstance().createWindow('another-window', {
    //   width: 800,
    //   height: 600,
    //   show: false,
    //   webPreferences: {
    //     nodeIntegration: true,
    //     devTools: false
    //   }
    // },
    // require('url').format({
    //   pathname: require('path').join(__dirname, './index.html'),
    //   protocol: 'file:',
    //   slashes: true
    // }),
    // './renderer/renderer.js')
}

module.exports = function bootstrap(k) {
    if (!Array.isArray(k) || k.length === 0) {
        throw new Error("Failed to bootstrap application.");
    }
    WindowManager.__SECRET_KEY__ = k;

    if (!process.env.ELECTRON_RUN_AS_NODE) {
        mustNotExportKey(k);
        if (app.whenReady === "function") {
            app.whenReady()
                .then(main)
                .catch((err) => console.log(err));
        } else {
            app.on("ready", main);
        }
    } else {
        console.log(k);
        assert.strictEqual(k.length, 32, "Key length error.");
        console.log("\nTest passed.\n");
    }
};
