/**
 * Adobe Edge: symbol definitions
 */
(function($, Edge, compId){
//images folder
var im='images/';

var fonts = {};
var opts = {
    'gAudioPreloadPreference': 'auto',

    'gVideoPreloadPreference': 'auto'
};
var resources = [
];
var symbols = {
"stage": {
    version: "4.0.0",
    minimumCompatibleVersion: "4.0.0",
    build: "4.0.0.359",
    baseState: "Base State",
    scaleToFit: "both",
    centerStage: "both",
    initialState: "Base State",
    gpuAccelerate: false,
    resizeInstances: false,
    content: {
            dom: [
            {
                id: 'Text',
                type: 'text',
                rect: ['385px', '317px','509px','86px','auto', 'auto'],
                text: "This is a test",
                align: "center",
                font: ['Trebuchet MS, Arial, Helvetica, sans-serif', 66, "rgba(0,0,0,1)", "400", "none", ""],
                textShadow: ["rgba(0,0,0,0.65098)", 3, 3, 3],
                filter: [0, 0, 1, 1, 0, 0, 0, 1, "rgba(0,0,0,0)", 0, 0, 0]
            },
            {
                id: 'Text2',
                type: 'text',
                rect: ['-133.2%', '-113.9%','160px','41px','auto', 'auto'],
                text: "link",
                align: "center",
                font: ['\'Trebuchet MS\', Arial, Helvetica, sans-serif', 66, "rgba(255,0,0,1)", "400", "none", "normal"]
            }],
            symbolInstances: [

            ]
        },
    states: {
        "Base State": {
            "${_Stage}": [
                ["color", "background-color", 'rgba(0,47,255,0.00)'],
                ["style", "width", '1280px'],
                ["style", "height", '720px'],
                ["style", "overflow", 'hidden']
            ],
            "${_Text}": [
                ["subproperty", "textShadow.blur", '3px'],
                ["subproperty", "textShadow.offsetH", '3px'],
                ["style", "-webkit-transform-origin", [50,50], {valueTemplate:'@@0@@% @@1@@%'} ],
                ["style", "-moz-transform-origin", [50,50],{valueTemplate:'@@0@@% @@1@@%'}],
                ["style", "-ms-transform-origin", [50,50],{valueTemplate:'@@0@@% @@1@@%'}],
                ["style", "msTransformOrigin", [50,50],{valueTemplate:'@@0@@% @@1@@%'}],
                ["style", "-o-transform-origin", [50,50],{valueTemplate:'@@0@@% @@1@@%'}],
                ["transform", "rotateZ", '0deg'],
                ["color", "color", 'rgba(255,0,0,1.00)'],
                ["style", "font-weight", '400'],
                ["style", "left", '385px'],
                ["style", "width", '509px'],
                ["style", "top", '317px'],
                ["subproperty", "filter.blur", '0px'],
                ["style", "text-align", 'center'],
                ["subproperty", "textShadow.color", 'rgba(0,0,0,0.65098)'],
                ["style", "height", '86px'],
                ["style", "font-family", 'Trebuchet MS, Arial, Helvetica, sans-serif'],
                ["subproperty", "textShadow.offsetV", '3px'],
                ["style", "font-size", '66px']
            ],
            "${_Text2}": [
                ["style", "top", '0%'],
                ["style", "height", '86px'],
                ["style", "left", '0%'],
                ["style", "width", '207px']
            ]
        }
    },
    timelines: {
        "Default Timeline": {
            fromState: "Base State",
            toState: "",
            duration: 5000,
            autoPlay: true,
            timeline: [
                { id: "eid26", tween: [ "style", "${_Text2}", "left", '0%', { fromValue: '0%'}], position: 0, duration: 0 },
                { id: "eid28", tween: [ "style", "${_Text2}", "height", '86px', { fromValue: '86px'}], position: 0, duration: 0 },
                { id: "eid3", tween: [ "transform", "${_Text}", "rotateZ", '360deg', { fromValue: '0deg'}], position: 0, duration: 2000 },
                { id: "eid27", tween: [ "style", "${_Text2}", "top", '0%', { fromValue: '0%'}], position: 0, duration: 0 },
                { id: "eid29", tween: [ "style", "${_Text2}", "width", '207px', { fromValue: '207px'}], position: 0, duration: 0 },
                { id: "eid24", tween: [ "color", "${_Stage}", "background-color", 'rgba(0,47,255,0.00)', { animationColorSpace: 'RGB', valueTemplate: undefined, fromValue: 'rgba(0,47,255,0.00)'}], position: 0, duration: 0 },
                { id: "eid22", tween: [ "subproperty", "${_Text}", "filter.blur", '33px', { fromValue: '0px'}], position: 0, duration: 1000 },
                { id: "eid23", tween: [ "subproperty", "${_Text}", "filter.blur", '0px', { fromValue: '33px'}], position: 1000, duration: 1000 }            ]
        }
    }
}
};


Edge.registerCompositionDefn(compId, symbols, fonts, resources, opts);

/**
 * Adobe Edge DOM Ready Event Handler
 */
$(window).ready(function() {
     Edge.launchComposition(compId);
});
})(jQuery, AdobeEdge, "EDGE-1050614614");
