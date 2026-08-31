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
    interface Series {
        /**
         * Get non-presentational attributes for a point. Used internally for
         * both styled mode and classic. Set correct position in link with
         * connector line.
         *
         * @param this
         *        The series of points.
         *
         * @return A hash containing those attributes that are not settable from
         *         CSS.
         */
        markerAttribs(this: Series): SVGAttributes;
    }
}
export default factory;
export let Highcharts: typeof _Highcharts;
