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
     * Extend generatePoints by adding the nodes, which are Point objects but
     * pushed to the this.nodes array.
     */
    function generatePoints(): void;
    /**
     * When hovering node, highlight all connected links. When hovering a link,
     * highlight all connected nodes.
     */
    function setNodeState(): void;
}
export default factory;
export let Highcharts: typeof _Highcharts;
