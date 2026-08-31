Highcharts is a JavaScript charting library based on SVG rendering. This project includes Stock, the financial charting package, the Maps package for geo maps and the Gantt package.

This package is intended for supporting client-side JavaScript charting through bundlers like Parcel or Webpack and environments like Babel or TypeScript. If you intend to generate static charts on the server side, use the [Highcharts node.js Export Server](https://www.npmjs.com/package/highcharts-export-server) instead.

### License
Please note that commercial use of Highcharts requires a commercial license. For testing and demonstration purposes (POC), Highcharts can be used free of charge. Non-profit organisations, schools and personal websites are qualified for the free license. Read more about licenses [in the Highcharts shop](https://shop.highsoft.com/?utm_source=npmjs&utm_medium=referral&utm_campaign=highchartspage&utm_content=licenseinfo).

### Links
* Official website: [www.highcharts.com](http://www.highcharts.com)
* Download page: [www.highcharts.com/download](http://www.highcharts.com/download)
* License: [www.highcharts.com/license](http://www.highcharts.com/license)
* Support: [www.highcharts.com/support](http://www.highcharts.com/support)
* Issues: [Working repo](https://github.com/highcharts/highcharts/issues)

## Download and install Highcharts
Please note that there are several ways to use Highcharts. For general installation instructions, see [the docs](http://www.highcharts.com/docs/getting-started/installation).
### Use our CDN
Instead of downloading, you can use our CDN to access files directly. See [code.highcharts.com](https://code.highcharts.com) for details.
```
<script src="https://code.highcharts.com/highcharts.js"></script>
```
### Install from npm
See [npm documentation](https://docs.npmjs.com/) on how to get started with npm.
```
npm install --save highcharts
```

#### Install nightly build
See [highcharts documentation](https://www.highcharts.com/docs/getting-started/install-from-npm) for installing the nightly build.

Note that we do not recommend the use of the nightly build in production environments as it **may contain bugs and is not considered stable.**

```
npm install --save highcharts/highcharts-dist#nightly
```


### Install from Bower
See [Bower documentation](https://bower.io/) on how to get started with Bower.
```
bower install highcharts
```

## Load Highcharts as a CommonJS module
Highcharts is using an UMD module pattern, as a result it has support for CommonJS.
*The following examples presumes you are using npm to install Highcharts, see [Download and install Highcharts](#download-and-install-highcharts) for more details.*
```js
// Load Highcharts
var Highcharts = require('highcharts');
// Alternatively, this is how to load Highcharts Stock. The Maps and Gantt
// packages are similar.
// var Highcharts = require('highcharts/highstock');

// Load the exporting module, and initialize it.
require('highcharts/modules/exporting')(Highcharts);

// Generate the chart
Highcharts.chart('container', {
  // options - see https://api.highcharts.com/highcharts
});
```

## Load Highcharts as an ES6 module
Since Highcharts supports CommonJS, it can be loaded as an ES6 module with the use of transpilers. Two common transpilers are [Babel](https://babeljs.io/) and [TypeScript](https://www.typescriptlang.org/). These have different interpretations of a CommonJS module, which affects your syntax.
*The following examples presumes you are using npm to install Highcharts, see [Download and install Highcharts](#download-and-install-highcharts) for more details.*
### Babel
```js
import Highcharts from 'highcharts';
// Alternatively, this is how to load Highcharts Stock. The Maps and Gantt
// packages are similar.
// import Highcharts from 'highcharts/highstock';

// Load the exporting module.
import Exporting from 'highcharts/modules/exporting';
// Initialize exporting module.
Exporting(Highcharts);

// Generate the chart
Highcharts.chart('container', {
  // options - see https://api.highcharts.com/highcharts
});
```
### TypeScript
```js
import * as Highcharts from 'highcharts';
// Alternatively, this is how to load Highcharts Stock. The Maps and Gantt
// packages are similar.
// import Highcharts from 'highcharts/highstock';

// Load the exporting module.
import * as Exporting from 'highcharts/modules/exporting';
// Initialize exporting module.
Exporting(Highcharts);

// Generate the chart
Highcharts.chart('container', {
  // options - see https://api.highcharts.com/highcharts
});
```

