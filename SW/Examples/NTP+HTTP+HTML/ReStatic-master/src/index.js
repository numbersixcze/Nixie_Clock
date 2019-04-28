import watch from 'node-watch';
import { minify } from 'html-minifier';
import fs from 'fs-extra';
import path from 'path';
import _ from 'lodash';
import Mustache from 'mustache';
import he from 'he';
import matchAll from 'match-all';

const variableTagPattern = /<% ([^%<>]+) %>/g;

// Logic

function buildOutput() {
    const pages = [];
    const pushToOutput = (pageName, pageVariables, pageContent) => {
        pages.push({
            name: pageName.charAt(0).toUpperCase() + pageName.slice(1),
            variables: _.join(pageVariables.map(variableName => `String ${variableName}`), ', '),
            content: pageContent
        });
    };

    console.log(`Building static pages from input directory...`);
    fs.readdirSync(process.env.PAGES_ROOT_FOLDER).forEach(rootFolderItem => {
        const rootFolderPath = path.join(process.env.PAGES_ROOT_FOLDER, rootFolderItem);

        if (fs.statSync(rootFolderPath).isDirectory()) {
            console.log(`Found page folder '${rootFolderPath}', composing page '${rootFolderItem}'...`);
            composePage(rootFolderPath, pushToOutput);
        }
    });

    console.log(`Building and saving output files...`);
    const hTemplate = fs.readFileSync('src/templates/h.mustache').toString();
    fs.outputFileSync(path.join(process.env.OUTPUT_FOLDER, 'Pages.h'), Mustache.render(hTemplate, { pages }));

    const cppTemplate = fs.readFileSync('src/templates/cpp.mustache').toString();
    fs.outputFileSync(path.join(process.env.OUTPUT_FOLDER, 'Pages.cpp'), Mustache.render(cppTemplate, { pages }));
    console.log(`Build complete!`);
}

function composePage(pageFolder, pushToOutput) {
    const pageFiles = {
        content: [],
        styles: [],
        scripts: []
    };

    const fileHandlers = {
        '.html': (filePath) => pageFiles.content.push(filePath),
        '.css': (filePath) => pageFiles.styles.push(filePath),
        '.js': (filePath) => pageFiles.scripts.push(filePath),
    };

    fs.readdirSync(pageFolder).forEach(pageItem => {
        const pageItemPath = path.join(pageFolder, pageItem);
        const fileHanlder = _.get(fileHandlers, path.extname(pageItem));

        if (fileHanlder) {
            console.log(`   - ${pageItem}`);
            fileHanlder(pageItemPath);
        }
    });
    console.log(`File gathering for ${pageFolder} complete!`);

    console.log(`Reading content files...`);
    const combinedContent = pageFiles.content
        .map(contentFile => fs.readFileSync(contentFile, 'utf8').toString())
        .join('');

    console.log(`Reading style files...`);
    const styles  = pageFiles.styles.map(
        styleFile => fs.readFileSync(styleFile, 'utf8').toString()
    );

    console.log(`Reading script files...`);
    const scripts  = pageFiles.scripts.map(
        scriptFile => fs.readFileSync(scriptFile, 'utf8').toString()
    );

    const pageData = {
        content: combinedContent,
        styles,
        scripts
    };

    buildPage(path.basename(pageFolder), pageData, pushToOutput);
}

function buildPage(name, data, pushToOutput) {
    console.log(`Reading page template file...`);
    const pageTemplate = fs.readFileSync('src/templates/page.mustache').toString();

    console.log(`Rendering data to the page template...`);
    const renderedPage = Mustache.render(pageTemplate, data);

    console.log(`Minifing output...`);
    const minificationConfig ={
        collapseWhitespace: true,
        minifyCSS: true,
        minifyJS: true,
        removeComments: true,
        removeEmptyAttributes: true,
        removeEmptyElements: true,
        removeRedundantAttributes: true
    };

    const minifiedPage = he.unescape(minify(renderedPage, minificationConfig)).replace(/"/g, "'");
    console.log(` = ${minifiedPage}`);

    console.log(`Replacing variable tags with appenders and formating page to valid string...`);
    const variables = matchAll(minifiedPage, variableTagPattern).toArray();
    console.log(` - Variables: ${variables}`);
    const outputPage = `"${minifiedPage.replace(variableTagPattern, '" + $1 + "')}"`;
    console.log(` - Output: ${outputPage}`);

    console.log(`Pushing built page '${name}' to output.`);
    pushToOutput(name, variables, outputPage);
}

// Init

console.log(`Ensuring existance of input and output folders...`);
fs.ensureDirSync(process.env.PAGES_ROOT_FOLDER);
fs.ensureDirSync(process.env.OUTPUT_FOLDER);

const watchConfig = {
    recursive: true,
    filter: /\.(?:html|css|js)$/
};

console.log(`Starting to watch for file chagnes at: ${process.env.PAGES_ROOT_FOLDER}...`);
watch(process.env.PAGES_ROOT_FOLDER, watchConfig, (changeType, filePath) => {
    console.log('Files changed, rebuinding...');
    buildOutput();
    console.log('');
});

buildOutput();