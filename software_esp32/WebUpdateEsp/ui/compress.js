const { gzip } = require('@gfx/zopfli');
const FS = require('fs');
const path = require('path');

const BUNDLE_JS = FS.readFileSync(path.resolve(__dirname, './dist/js/app.js'));

const HTML = `
<!DOCTYPE html>
<html lang=en>

<head>
    <meta charset=utf-8>
    <meta http-equiv=X-UA-Compatible content="IE=edge">
    <meta name=viewport content="width=device-width,initial-scale=1">
    <link rel="icon" href="data:,"> 
	<title>WT32 Update OTA</title>
 
</head>

<body style="overflow: hidden;">
    <noscript>
        <strong>We're sorry but WT32 OTA doesn't work properly without JavaScript enabled. Please enable it to continue.</strong>
    </noscript>
    <div id=app></div>
    <script defer>${BUNDLE_JS}</script>
</body>
</html>
`;

function chunkArray(myArray, chunk_size) {
  let index = 0;
  const arrayLength = myArray.length;
  const tempArray = [];
  for (index = 0; index < arrayLength; index += chunk_size) {
    myChunk = myArray.slice(index, index + chunk_size);
    // Do something if you want with the group
    tempArray.push(myChunk);
  }
  return tempArray;
}

function addLineBreaks(buffer) {
  let data = '';
  const chunks = chunkArray(buffer, 30);
  chunks.forEach((chunk, index) => {
    data += chunk.join(',');
    if (index + 1 !== chunks.length) {
      data += ',\n';
    }
  });
  return data;
}


gzip(HTML, { numiterations: 15 }, (err, output) => {
  if (err) {
    return console.error('error on gzip : '+err);
  }

  const FILE = `#pragma once

const uint32_t WT32Upate_HTML_SIZE = ${output.length};
const uint8_t WT32Upate_HTML[] PROGMEM = { 
${addLineBreaks(output)} 
};

`;

  FS.writeFileSync(path.resolve(__dirname, '../../include/WT32UpdateWebpage.h'), FILE);
  console.log(`[COMPRESS] Compressed Build Files to WT32UpdateWebpage.h: ${ (output.length/1024).toFixed(2) }KB`);
});
