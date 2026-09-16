'use strict';

var fs = require('fs');
var path = require('path');
var zlib = require('zlib');

var angularDir = path.join(__dirname, '..', 'node_modules', 'angular');
var sourcePath = path.join(angularDir, 'angular.js');
var minPath = path.join(angularDir, 'angular.min.js');

function patch(filePath, marker, replacement, patchedMarker) {
  var source = fs.readFileSync(filePath, 'utf8');
  if (source.indexOf(patchedMarker) !== -1) {
    return;
  }
  if (source.indexOf(marker) === -1) {
    throw new Error('Unsupported AngularJS source: ' + filePath);
  }
  fs.writeFileSync(filePath, source.replace(marker, replacement));
}

var sourceMarker =
    "      if (!isString(value)) {\n" +
    "        throw $compileMinErr('srcset', 'Can\\'t pass trusted values to `{0}`: \"{1}\"', invokeType, value.toString());\n" +
    "      }\n";
var sourceReplacement =
    sourceMarker +
    "      if (value.length > 4096) {\n" +
    "        throw $compileMinErr('srcsetlen', '`{0}` exceeds the maximum supported srcset length', invokeType);\n" +
    "      }\n";

patch(sourcePath, sourceMarker, sourceReplacement, "if (value.length > 4096)");

var minMarker = 'if(!C(a))throw $("srcset",b,a.toString());for(var c=""';
var minReplacement =
    'if(!C(a))throw $("srcset",b,a.toString());' +
    'if(4096<a.length)throw $("srcsetlen",b);for(var c=""';

patch(minPath, minMarker, minReplacement, '4096<a.length');
fs.writeFileSync(minPath + '.gzip', zlib.gzipSync(fs.readFileSync(minPath)));
