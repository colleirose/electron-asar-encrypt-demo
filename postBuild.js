const asarmor = require("asarmor");
const fs = require("fs");
const path = require("path");

/*

For some reason, the node file that ends up in resources/app.asar.unpacked/main.node is sometimes an old one from an earlier build, even though the right version is in literally every other location, including the build folder. I cannot tell why this is or where else the earlier version's file is even being copied from, but this is the only working solution I can find.

TODO: Check if this was because of the app folder instead of appbuild folder and by extension its outdated node files being referenced in the past.

*/

console.log("Fixing outdated node files");

(async () => {
    fs.unlink(
        path.join(__dirname, "./test/resources/app.asar.unpacked/main.node"),
        (err) => {
            if (err) throw err;
            console.log("Unlinked main.node");
        }
    );

    fs.unlink(
        path.join(
            __dirname,
            "./test/resources/app.asar.unpacked/renderer.node"
        ),
        (err) => {
            if (err) throw err;
            console.log("Unlinked renderer.node");
        }
    );

    fs.copyFile(
        path.join(__dirname, "./build/Release/main.node"),
        path.join(__dirname, "./test/resources/app.asar.unpacked/main.node"),
        (err) => {
            if (err) throw err;
            console.log("Updated main.node");
        }
    );

    fs.copyFile(
        path.join(__dirname, "./build/Release/renderer.node"),
        path.join(
            __dirname,
            "./test/resources/app.asar.unpacked/renderer.node"
        ),
        (err) => {
            if (err) throw err;
            console.log("Updated renderer.node");
        }
    );
})();

// Apply asarmor to the asar file

const generateRandomString = (length) => {
    // https://stackoverflow.com/a/1349426
    var result = "";
    var characters =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    var charactersLength = characters.length;

    for (var i = 0; i < length; i++) {
        result += characters.charAt(
            Math.floor(Math.random() * charactersLength)
        );
    }

    return result;
};

const randomNumber = (min, max) => {
    Math.floor(Math.random() * max) + min;
};

const applyPatch = async (file) => {
    console.log(`Applying asarmor to ${file}`);

    const archive = await asarmor.open(file);

    if (file.includes("node_modules")) {
        archive.patch(
            asarmor.createTrashPatch({
                filenames: [
                    generateRandomString(randomNumber(15, 1)),
                    generateRandomString(randomNumber(15, 1)),
                    generateRandomString(randomNumber(15, 1)),
                    generateRandomString(randomNumber(15, 1)),
                    generateRandomString(randomNumber(15, 1)),
                    generateRandomString(randomNumber(15, 1)),
                    generateRandomString(randomNumber(15, 1)),
                    generateRandomString(randomNumber(15, 1))
                ]
            })
        );
    } else {
        archive.patch(
            asarmor.createTrashPatch({
                filenames: [
                    "helper.node",
                    "package.json",
                    "package-lock.json",
                    "settings.json",
                    "index.tsx",
                    "header.tsx",
                    "button.tsx",
                    generateRandomString(randomNumber(15, 1))
                ]
            })
        );
    }

    // Apply customized bloat patch.
    // The bloat patch by itself will write randomness to disk on extraction attempt.
    archive.patch(asarmor.createBloatPatch(1000)); // adds 1,000 GB of bloat in total

    // Write changes back to disk.
    const outputPath = await archive.write(file);

    console.log(`Applied asarmor to ${outputPath}`);
};

applyPatch(path.join(__dirname, "./test/resources/app.asar"));
applyPatch(path.join(__dirname, "./test/resources/node_modules.asar"));
