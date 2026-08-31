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
         * Cancel current sonification and reset cursor.
         *
         * @param fadeOut
         *        Fade out as we pause to avoid clicks.
         */
        cancelSonify(fadeOut?: boolean): void;
        /**
         * Get a list of the points currently under cursor.
         *
         * @return The points currently under the cursor.
         */
        getCurrentSonifyPoints(): Array<Point>;
        /**
         * Pause the running sonification.
         *
         * @param fadeOut
         *        Fade out as we pause to avoid clicks.
         */
        pauseSonify(fadeOut?: boolean): void;
        /**
         * Reset cursor to start. Requires series.sonify or chart.sonify to have
         * been played at some point earlier.
         */
        resetSonifyCursor(): void;
        /**
         * Reset cursor to end. Requires series.sonify or chart.sonify to have
         * been played at some point earlier.
         */
        resetSonifyCursorEnd(): void;
        /**
         * Resume the currently running sonification. Requires series.sonify or
         * chart.sonify to have been played at some point earlier.
         *
         * @param onEnd
         *        Callback to call when play finished.
         */
        resumeSonify(onEnd: Function): void;
        /**
         * Play backwards from cursor. Requires series.sonify or chart.sonify to
         * have been played at some point earlier.
         *
         * @param onEnd
         *        Callback to call when play finished.
         */
        rewindSonify(onEnd: Function): void;
        /**
         * Set the cursor to a point or set of points in different series.
         *
         * @param points
         *        The point or points to set the cursor to. If setting multiple
         *        points under the cursor, the points have to be in different
         *        series that are being played simultaneously.
         */
        setSonifyCursor(points: (Point|Array<Point>)): void;
        /**
         * Sonify a chart.
         *
         * @param options
         *        The options for sonifying this chart. If not provided, uses
         *        options set on chart and series.
         */
        sonify(options?: SonificationOptions): void;
    }
    /**
     * An Earcon configuration, specifying an Earcon and when to play it.
     */
    interface EarconConfiguration {
        /**
         * A function to determine whether or not to play this earcon on a
         * point. The function is called for every point, receiving that point
         * as parameter. It should return either a boolean indicating whether or
         * not to play the earcon, or a new Earcon instance - in which case the
         * new Earcon will be played.
         */
        condition?: Function;
        /**
         * An Earcon instance.
         */
        earcon: Earcon;
        /**
         * The ID of the point to play the Earcon on.
         */
        onPoint?: string;
    }
    /**
     * Define an Instrument and the options for playing it.
     */
    interface EarconInstrument {
        /**
         * An instrument instance or the name of the instrument in the
         * Highcharts.sonification.instruments map.
         */
        instrument: (string|Instrument);
        /**
         * The options to pass to Instrument.play.
         */
        playOptions: InstrumentPlayOptionsObject;
    }
    /**
     * Options for an Earcon.
     */
    interface EarconOptionsObject {
        /**
         * The unique ID of the Earcon. Generated if not supplied.
         */
        id?: string;
        /**
         * The instruments and their options defining this earcon.
         */
        instruments: Array<EarconInstrument>;
        /**
         * Callback function to call when earcon has finished playing.
         */
        onEnd?: Function;
        /**
         * Global panning of all instruments. Overrides all panning on
         * individual instruments. Can be a number between -1 and 1.
         */
        pan?: number;
        /**
         * Master volume for all instruments. Volume settings on individual
         * instruments can still be used for relative volume between the
         * instruments. This setting does not affect volumes set by functions in
         * individual instruments. Can be a number between 0 and 1. Defaults to
         * 1.
         */
        volume?: number;
    }
    /**
     * A set of options for the Instrument class.
     */
    interface InstrumentOptionsObject {
        /**
         * A list of allowed frequencies for this instrument. If trying to play
         * a frequency not on this list, the closest frequency will be used. Set
         * to `null` to allow all frequencies to be used. Defaults to `null`.
         */
        allowedFrequencies?: Array<number>;
        /**
         * The unique ID of the instrument. Generated if not supplied.
         */
        id?: string;
        /**
         * Options specific to oscillator instruments.
         */
        oscillator?: OscillatorOptionsObject;
        /**
         * When using functions to determine frequency or other parameters
         * during playback, this options specifies how often to call the
         * callback functions. Number given in milliseconds. Defaults to 20.
         */
        playCallbackInterval?: number;
        /**
         * The type of instrument. Currently only `oscillator` is supported.
         * Defaults to `oscillator`.
         */
        type?: string;
    }
    /**
     * Options for playing an instrument.
     */
    interface InstrumentPlayOptionsObject {
        /**
         * The duration of the note in milliseconds.
         */
        duration: number;
        /**
         * The frequency of the note to play. Can be a fixed number, or a
         * function. The function receives one argument: the relative time of
         * the note playing (0 being the start, and 1 being the end of the
         * note). It should return the frequency number for each point in time.
         * The poll interval of this function is specified by the
         * Instrument.playCallbackInterval option.
         */
        frequency: (number|Function);
        /**
         * The master volume multiplier to apply to the instrument, regardless
         * of other volume changes. Defaults to 1.
         */
        masterVolume?: number;
        /**
         * The maximum frequency to allow. If the instrument has a set of
         * allowed frequencies, the closest frequency is used by default. Use
         * this option to stop too high frequencies from being used.
         */
        maxFrequency?: number;
        /**
         * The minimum frequency to allow. If the instrument has a set of
         * allowed frequencies, the closest frequency is used by default. Use
         * this option to stop too low frequencies from being used.
         */
        minFrequency?: number;
        /**
         * Callback function to be called when the play is completed.
         */
        onEnd?: Function;
        /**
         * The panning of the instrument. Can be a fixed number between -1 and
         * 1, or a function. The function receives one argument: the relative
         * time of the note playing (0 being the start, and 1 being the end of
         * the note). It should return the panning value for each point in time.
         * The poll interval of this function is specified by the
         * Instrument.playCallbackInterval option. Defaults to 0.
         */
        pan?: (number|Function);
        /**
         * The volume of the instrument. Can be a fixed number between 0 and 1,
         * or a function. The function receives one argument: the relative time
         * of the note playing (0 being the start, and 1 being the end of the
         * note). It should return the volume for each point in time. The poll
         * interval of this function is specified by the
         * Instrument.playCallbackInterval option. Defaults to 1.
         */
        volume?: (number|Function);
    }
    interface OscillatorOptionsObject {
        /**
         * The waveform shape to use for oscillator instruments. Defaults to
         * `sine`.
         */
        waveformShape?: string;
    }
    interface Point {
        /**
         * Cancel sonification of a point. Calls onEnd functions.
         *
         * @param fadeOut
         *        Whether or not to fade out as we stop. If false, the points
         *        are cancelled synchronously.
         */
        cancelSonify(fadeOut?: boolean): void;
        /**
         * Sonify a single point.
         *
         * @param options
         *        Options for the sonification of the point.
         */
        sonify(options: PointSonifyOptionsObject): void;
    }
    /**
     * Define the parameter mapping for an instrument.
     */
    interface PointInstrumentMappingObject {
        /**
         * Define the duration of the notes for this instrument. This can be a
         * string with a data property name, e.g. `'y'`, in which case this data
         * property is used to define the duration relative to the `y`-values of
         * the other points. A higher `y` value would then result in a longer
         * duration. Alternatively, `'-y'` can be used, in which case the
         * polarity is inverted, and a higher `y` value would result in a
         * shorter duration. This option can also be a fixed number or a
         * function. If it is a function, this function is called once before
         * the note starts playing, and should return the duration in
         * milliseconds. It receives two arguments: The point, and the
         * dataExtremes.
         */
        duration: (number|string|Function);
        /**
         * Define the frequency of the instrument. This can be a string with a
         * data property name, e.g. `'y'`, in which case this data property is
         * used to define the frequency relative to the `y`-values of the other
         * points. A higher `y` value would then result in a higher frequency.
         * Alternatively, `'-y'` can be used, in which case the polarity is
         * inverted, and a higher `y` value would result in a lower frequency.
         * This option can also be a fixed number or a function. If it is a
         * function, this function is called in regular intervals while the note
         * is playing. It receives three arguments: The point, the dataExtremes,
         * and the current relative time - where 0 is the beginning of the note
         * and 1 is the end. The function should return the frequency of the
         * note as a number (in Hz).
         */
        frequency: (number|string|Function);
        /**
         * Define the panning of the instrument. This can be a string with a
         * data property name, e.g. `'x'`, in which case this data property is
         * used to define the panning relative to the `x`-values of the other
         * points. A higher `x` value would then result in a higher panning
         * value (panned further to the right). Alternatively, `'-x'` can be
         * used, in which case the polarity is inverted, and a higher `x` value
         * would result in a lower panning value (panned further to the left).
         * This option can also be a fixed number or a function. If it is a
         * function, this function is called in regular intervals while the note
         * is playing. It receives three arguments: The point, the dataExtremes,
         * and the current relative time - where 0 is the beginning of the note
         * and 1 is the end. The function should return the panning of the note
         * as a number between -1 and 1.
         */
        pan?: (number|string|Function);
        /**
         * Define the volume of the instrument. This can be a string with a data
         * property name, e.g. `'y'`, in which case this data property is used
         * to define the volume relative to the `y`-values of the other points.
         * A higher `y` value would then result in a higher volume.
         * Alternatively, `'-y'` can be used, which inverts the polarity, so
         * that a higher `y` value results in a lower volume. This option can
         * also be a fixed number or a function. If it is a function, this
         * function is called in regular intervals while the note is playing. It
         * receives three arguments: The point, the dataExtremes, and the
         * current relative time - where 0 is the beginning of the note and 1 is
         * the end. The function should return the volume of the note as a
         * number between 0 and 1.
         */
        volume: (number|string|Function);
    }
    /**
     * An instrument definition for a point, specifying the instrument to play
     * and how to play it.
     */
    interface PointInstrumentObject {
        /**
         * An Instrument instance or the name of the instrument in the
         * Highcharts.sonification.instruments map.
         */
        instrument: (string|Instrument);
        /**
         * Mapping of instrument parameters for this instrument.
         */
        instrumentMapping: PointInstrumentMappingObject;
        /**
         * Options for this instrument.
         */
        instrumentOptions?: PointInstrumentOptionsObject;
        /**
         * Callback to call when the instrument has stopped playing.
         */
        onEnd?: Function;
    }
    interface PointInstrumentOptionsObject {
        /**
         * The maximum duration for a note when using a data property for
         * duration. Can be overridden by using either a fixed number or a
         * function for instrumentMapping.duration. Defaults to 2000.
         */
        maxDuration?: number;
        /**
         * The maximum frequency for a note when using a data property for
         * frequency. Can be overridden by using either a fixed number or a
         * function for instrumentMapping.frequency. Defaults to 2200.
         */
        maxFrequency?: number;
        /**
         * The maximum pan value for a note when using a data property for
         * panning. Can be overridden by using either a fixed number or a
         * function for instrumentMapping.pan. Defaults to 1 (fully right).
         */
        maxPan?: number;
        /**
         * The maximum volume for a note when using a data property for volume.
         * Can be overridden by using either a fixed number or a function for
         * instrumentMapping.volume. Defaults to 1.
         */
        maxVolume?: number;
        /**
         * The minimum duration for a note when using a data property for
         * duration. Can be overridden by using either a fixed number or a
         * function for instrumentMapping.duration. Defaults to 20.
         */
        minDuration?: number;
        /**
         * The minimum frequency for a note when using a data property for
         * frequency. Can be overridden by using either a fixed number or a
         * function for instrumentMapping.frequency. Defaults to 220.
         */
        minFrequency?: number;
        /**
         * The minimum pan value for a note when using a data property for
         * panning. Can be overridden by using either a fixed number or a
         * function for instrumentMapping.pan. Defaults to -1 (fully left).
         */
        minPan?: number;
        /**
         * The minimum volume for a note when using a data property for volume.
         * Can be overridden by using either a fixed number or a function for
         * instrumentMapping.volume. Defaults to 0.1.
         */
        minVolume?: number;
    }
    /**
     * Options for sonifying a point.
     */
    interface PointSonifyOptionsObject {
        /**
         * Optionally provide the minimum/maximum values for the points. If this
         * is not supplied, it is calculated from the points in the chart on
         * demand. This option is supplied in the following format, as a map of
         * point data properties to objects with min/max values: (see online
         * documentation for example)
         */
        dataExtremes?: object;
        /**
         * The instrument definitions for this point.
         */
        instruments: Array<PointInstrumentObject>;
        /**
         * Callback called when the sonification has finished.
         */
        onEnd?: Function;
    }
    interface Series {
        /**
         * Sonify a series.
         *
         * @param options
         *        The options for sonifying this series. If not provided, uses
         *        options set on chart and series.
         */
        sonify(options?: SonifySeriesOptionsObject): void;
    }
    /**
     * Global classes and objects related to sonification.
     */
    interface SonificationObject {
        /**
         * The Earcon class.
         */
        Earcon: Function;
        /**
         * Note fade-out-time in milliseconds. Most notes are faded out quickly
         * by default if there is time. This is to avoid abrupt stops which will
         * cause perceived clicks.
         */
        fadeOutDuration: number;
        /**
         * The Instrument class.
         */
        Instrument: Function;
        /**
         * Predefined instruments, given as an object with a map between the
         * instrument name and the Highcharts.Instrument object.
         */
        instruments: object;
    }
    /**
     * Options for sonifying a chart.
     */
    interface SonificationOptions {
        /**
         * Milliseconds of silent waiting to add between series. Note that
         * waiting time is considered part of the sonify duration.
         */
        afterSeriesWait?: number;
        /**
         * Optionally provide the minimum/maximum data values for the points. If
         * this is not supplied, it is calculated from all points in the chart
         * on demand. This option is supplied in the following format, as a map
         * of point data properties to objects with min/max values: (see online
         * documentation for example)
         */
        dataExtremes?: Dictionary<RangeObject>;
        /**
         * Duration for sonifying the entire chart. The duration is distributed
         * across the different series intelligently, but does not take earcons
         * into account. It is also possible to set the duration explicitly per
         * series, using `seriesOptions`. Note that points may continue to play
         * after the duration has passed, but no new points will start playing.
         */
        duration: number;
        /**
         * Earcons to add to the chart. Note that earcons can also be added per
         * series using `seriesOptions`.
         */
        earcons?: Array<EarconConfiguration>;
        /**
         * The instrument definitions for the points in this chart.
         */
        instruments?: Array<PointInstrumentObject>;
        /**
         * Callback after the chart has played.
         */
        onEnd?: Function;
        /**
         * Callback after a series has finished playing.
         */
        onSeriesEnd?: Function;
        /**
         * Callback before a series is played.
         */
        onSeriesStart?: Function;
        /**
         * Define the order to play the series in. This can be given as a
         * string, or an array specifying a custom ordering. If given as a
         * string, valid values are `sequential` - where each series is played
         * in order - or `simultaneous`, where all series are played at once.
         * For custom ordering, supply an array as the order. Each element in
         * the array can be either a string with a series ID, an Earcon object,
         * or an object with a numeric `silentWait` property designating a
         * number of milliseconds to wait before continuing. Each element of the
         * array will be played in order. To play elements simultaneously, group
         * the elements in an array.
         */
        order: (string|Array<(string|Earcon|Array<(string|Earcon)>)>);
        /**
         * The axis to use for when to play the points. Can be a string with a
         * data property (e.g. `x`), or a function. If it is a function, this
         * function receives the point as argument, and should return a numeric
         * value. The points with the lowest numeric values are then played
         * first, and the time between points will be proportional to the
         * distance between the numeric values. This option can not be
         * overridden per series.
         */
        pointPlayTime: (string|Function);
        /**
         * Options as given to `series.sonify` to override options per series.
         * If the option is supplied as an array of options objects, the `id`
         * property of the object should correspond to the series' id. If the
         * option is supplied as a single object, the options apply to all
         * series.
         */
        seriesOptions?: (object|Array<object>);
    }
    /**
     * Options for sonifying a series.
     */
    interface SonifySeriesOptionsObject {
        /**
         * Optionally provide the minimum/maximum data values for the points. If
         * this is not supplied, it is calculated from all points in the chart
         * on demand. This option is supplied in the following format, as a map
         * of point data properties to objects with min/max values: (see online
         * documentation for example)
         */
        dataExtremes?: Dictionary<RangeObject>;
        /**
         * The duration for playing the points. Note that points might continue
         * to play after the duration has passed, but no new points will start
         * playing.
         */
        duration: number;
        /**
         * Earcons to add to the series.
         */
        earcons?: Array<EarconConfiguration>;
        /**
         * The instrument definitions for the points in this series.
         */
        instruments: Array<PointInstrumentObject>;
        /**
         * Callback after the series has played.
         */
        onEnd?: Function;
        /**
         * Callback after a point has finished playing.
         */
        onPointEnd?: Function;
        /**
         * Callback before a point is played.
         */
        onPointStart?: Function;
        /**
         * The axis to use for when to play the points. Can be a string with a
         * data property (e.g. `x`), or a function. If it is a function, this
         * function receives the point as argument, and should return a numeric
         * value. The points with the lowest numeric values are then played
         * first, and the time between points will be proportional to the
         * distance between the numeric values.
         */
        pointPlayTime: (string|Function);
    }
    /**
     * The Earcon class. Earcon objects represent a certain sound consisting of
     * one or more instruments playing a predefined sound.
     */
    class Earcon {
        /**
         * The Earcon class. Earcon objects represent a certain sound consisting
         * of one or more instruments playing a predefined sound.
         *
         * @param options
         *        Options for the Earcon instance.
         */
        constructor(options: EarconOptionsObject);
        /**
         * Cancel any current sonification of the Earcon. Calls onEnd functions.
         *
         * @param fadeOut
         *        Whether or not to fade out as we stop. If false, the earcon is
         *        cancelled synchronously.
         */
        cancelSonify(fadeOut?: boolean): void;
        /**
         * Play the earcon, optionally overriding init options.
         *
         * @param options
         *        Override existing options.
         */
        sonify(options: EarconOptionsObject): void;
    }
    /**
     * The Instrument class. Instrument objects represent an instrument capable
     * of playing a certain pitch for a specified duration.
     */
    class Instrument {
        /**
         * The Instrument class. Instrument objects represent an instrument
         * capable of playing a certain pitch for a specified duration.
         *
         * @param options
         *        Options for the instrument instance.
         */
        constructor(options: InstrumentOptionsObject);
        /**
         * Return a copy of an instrument. Only one instrument instance can play
         * at a time, so use this to get a new copy of the instrument that can
         * play alongside it. The new instrument copy will receive a new ID
         * unless one is supplied in options.
         *
         * @param options
         *        Options to merge in for the copy.
         *
         * @return A new Instrument instance with the same options.
         */
        copy(options?: InstrumentOptionsObject): Instrument;
        /**
         * Mute an instrument that is playing. If the instrument is not
         * currently playing, this function does nothing.
         */
        mute(): void;
        /**
         * Play the instrument according to options.
         *
         * @param options
         *        Options for the playback of the instrument.
         */
        play(options: InstrumentPlayOptionsObject): void;
        /**
         * Stop the instrument playing.
         *
         * @param immediately
         *        Whether to do the stop immediately or fade out.
         *
         * @param onStopped
         *        Callback function to be called when the stop is completed.
         *
         * @param callbackData
         *        Data to send to the onEnd callback functions.
         */
        stop(immediately: boolean, onStopped?: Function, callbackData?: any): void;
    }
    /**
     * Global classes and objects related to sonification.
     */
    let sonification: SonificationObject;
}
export default factory;
export let Highcharts: typeof _Highcharts;
