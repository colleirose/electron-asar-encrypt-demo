const fs = require("fs-extra");
const path = require("path");
const JavaScriptObfuscator = require("javascript-obfuscator");
const htmlCompress = require("html-minifier").minify;
const { minify } = require("terser");

const appFolder = path.join(__dirname, "app");
const appRendererFolder = path.join(appFolder, "renderer");
const appBuildFolder = path.join(__dirname, "appbuild");

(async () => {
  try {
    await fs.emptyDir(appBuildFolder);
    console.log("Created empty folder", appBuildFolder);

    console.log(`Copying ${appFolder} to ${appBuildFolder}`);
    await fs.copy(appFolder, appBuildFolder, { overwrite: true });
    console.log(`Copied ${appFolder} to ${appBuildFolder}`);

    console.log("Minifying index.html");

    await fs.writeFile(
      path.join(appBuildFolder, "index.html"),
      htmlCompress(
        await fs.readFile(path.join(appBuildFolder, "index.html"), "utf-8"),
        {
          removeAttributeQuotes: true,
          removeComments: true,
          collapseWhitespace: true
        }
      )
    );

    console.log("Minified index.html");

    const obfuscateFilesInDirectory = async (folder) => {
      console.log(`Applying obfuscation to files in ${folder}`);

      try {
        const files = await fs.readdir(folder);
        await Promise.all(
          files.map(async (file) => {
            if (!file.endsWith(".js")) return;

            const filePath = path.join(folder, file);
            const buildFilePath = filePath.replace("app", "appbuild");
            let fileContent = await fs.readFile(filePath, "utf-8");

            // Minify and obfuscate content
            // Just using javascript-obfuscator on its own is easily reversible with public tools, but I've found this makes a result
            // that is both hard to reverse and not very large

            console.log(`Minifying and obfuscating ${file}`);
            fileContent = (await minify(fileContent)).code;
            fileContent = JavaScriptObfuscator.obfuscate(fileContent, {
              target: "node",
              rotateStringArray: true,
              controlFlowFlattening: true,
              controlFlowFlatteningThreshold: 0.3,
              deadCodeInjection: true,
              deadCodeInjectionThreshold: 0.4,
              numbersToExpressions: true,
              selfDefending: false,
              splitStrings: true,
              splitStringsChunkLength: 6,
              stringArrayWrappersCount: 5,
              stringArrayCallsTransform: true,
              stringArrayCallsTransformThreshold: 0.6,
              stringArrayEncoding: ["base64", "rc4", "none"],
              transformObjectKeys: true,
              identifierNamesGenerator: "mangled-shuffled"
            }).getObfuscatedCode();

            // Second round of minification and obfuscation
            fileContent = (await minify(fileContent)).code;
            fileContent = JavaScriptObfuscator.obfuscate(fileContent, {
              target: "node",
              rotateStringArray: true,
              controlFlowFlattening: true,
              controlFlowFlatteningThreshold: 0.5,
              deadCodeInjection: true,
              deadCodeInjectionThreshold: 0.2,
              numbersToExpressions: true,
              selfDefending: true,
              splitStrings: true,
              splitStringsChunkLength: 6,
              stringArrayWrappersCount: 5,
              stringArrayCallsTransform: true,
              stringArrayCallsTransformThreshold: 0.6,
              stringArrayEncoding: ["base64", "none"],
              transformObjectKeys: true,
              unicodeEscapeSequence: true,
              identifierNamesGenerator: "mangled-shuffled"
            }).getObfuscatedCode();

            await fs.writeFile(buildFilePath, fileContent, "utf-8");
            console.log(`Done minifying and obfuscating ${file}`);
          })
        );
        console.log(`Done applying obfuscation to files in ${folder}`);
      } catch (err) {
        console.error(`Error processing files in ${folder}:`, err);
      }
    };

    await obfuscateFilesInDirectory(appFolder);
    await obfuscateFilesInDirectory(appRendererFolder);
  } catch (err) {
    console.error("Error during build process:", err);
  }
})();
