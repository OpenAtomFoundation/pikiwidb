/*!*
 *
 *  Copyright (c) Highsoft AS. All rights reserved.
 *
 *!*/
import * as globals from "../globals.src";
import * as _Highcharts from "../highcharts.src";
/**
 * Adds the module to the imported Highcharts namespace.
 *
 * @param highcharts
 *        The imported Highcharts namespace to extend.
 */
export function factory(highcharts: typeof Highcharts): void;
declare module "../highcharts.src" {
    interface Chart {
        /**
         * Flag used in parallel coordinates plot to check if chart has
         * ||-coords (parallel coords).
         */
        hasParallelCoordinates: boolean;
    }
}
export default factory;
export let Highcharts: typeof _Highcharts;
