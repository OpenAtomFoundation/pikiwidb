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
    /**
     * Create the path based on points provided as argument. If
     * indicator.nextPoints option is defined, create the areaFill.
     *
     * @param points
     *        Points on which the path should be created
     */
    function getGraphPath(points: any): void;
    /**
     * Generate the API name of the line
     *
     * @param propertyName
     *        name of the line
     */
    function getLineName(propertyName: any): void;
}
export default factory;
export let Highcharts: typeof _Highcharts;
