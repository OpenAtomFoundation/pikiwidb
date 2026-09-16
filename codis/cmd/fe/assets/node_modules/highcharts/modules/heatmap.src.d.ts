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
    interface Point {
        /**
         * Heatmap series only. Padding between the points in the heatmap.
         */
        pointPadding?: number;
        /**
         * Heatmap series only. The value of the point, resulting in a color
         * controled by options as set in the colorAxis configuration.
         */
        value?: (number|null);
    }
    interface PointOptionsObject {
        /**
         * Heatmap series only. Point padding for a single point.
         */
        pointPadding?: number;
        /**
         * Heatmap series only. The value of the point, resulting in a color
         * controled by options as set in the colorAxis configuration.
         */
        value?: (number|null);
    }
}
export default factory;
export let Highcharts: typeof _Highcharts;
