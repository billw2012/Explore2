/***********************
* Adobe Edge Animate Composition Actions
*
* Edit this file with caution, being careful to preserve 
* function signatures and comments starting with 'Edge' to maintain the 
* ability to interact with these actions from within Adobe Edge Animate
*
***********************/
(function($, Edge, compId){
var Composition = Edge.Composition, Symbol = Edge.Symbol; // aliases for commonly used Edge classes

   //Edge symbol: 'stage'
   (function(symbolName) {
      
      
      Symbol.bindSymbolAction(compId, symbolName, "creationComplete", function(sym, e) {
         sym.$('Stage').css({
         '-webkit-touch-callout': 'none',
         '-webkit-user-select': 'none',
         '-khtml-user-select': 'none',
         '-moz-user-select': 'none',
         '-ms-user-select': 'none',
         'user-select':'none'
         });

      });
      //Edge binding end

      Symbol.bindElementAction(compId, symbolName, "document", "compositionReady", function(sym, e) {
         $.getJSON("menu_data.json")
         	.success(
         		function(data){
         			console.log("incoming data: ", data);
         
         			$.each(data, function(index, item) {
         				var s = sym.createChildSymbol("MainMenuEntry", "MenuItems");
         				s.$("Text").html(item.text);
         				s.$("Description").html(item.description);
         				s.play(index * -100);
         			});
         		}
         	);

      });
      //Edge binding end

   })("stage");
   //Edge symbol end:'stage'

   //=========================================================
   
   //Edge symbol: 'MainMenuEntry'
   (function(symbolName) {   
   
      Symbol.bindTriggerAction(compId, symbolName, "Default Timeline", 500, function(sym, e) {
         
         sym.stop();
         

      });
      //Edge binding end

      Symbol.bindTriggerAction(compId, symbolName, "Default Timeline", 562, function(sym, e) {
         // insert code here
         sym.stop();

      });
      //Edge binding end

      

      

      Symbol.bindElementAction(compId, symbolName, "${_BackgroundRect}", "mouseenter", function(sym, e) {
         sym.stop("Unselected");
         sym.play("Unselected");

      });
      //Edge binding end

      Symbol.bindElementAction(compId, symbolName, "${_BackgroundRect}", "mouseleave", function(sym, e) {
         sym.stop("Selected");
         sym.playReverse("Selected");
         

      });
      //Edge binding end

      Symbol.bindTimelineAction(compId, symbolName, "Default Timeline", "update", function(sym, e) {
         //console.log("MainMenuEntry.height:", theStage.getSymbol("MainMenuEntry").style.height);
         console.log("HeightRect.height = ", sym.$("HeightRect").height());
         //sym.$("HeightRect").parent().height( sym.$("HeightRect").height() );
         console.log("HeightRect.parent.height = ", sym.$("HeightRect").parent().height());
         var hgt = sym.$("HeightRect").height();
         console.log("hgt = ", hgt);
         //sym.$("HeightRect").parent().height( hgt );
         sym.$("HeightRect").parent().css({height: hgt});
         //sym.$("BackgroundRect").parent().css({height: hgt});
         

      });
      //Edge binding end

   })("MainMenuEntry");
   //Edge symbol end:'MainMenuEntry'

})(jQuery, AdobeEdge, "EDGE-1117821532");