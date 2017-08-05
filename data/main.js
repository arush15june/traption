var comments;
/*function generateCommentHMTML(msg,author) {
	return `<div class = "row"> 
    <div class = "col-xs-12 col-sm-12 col-md-6 col-lg-6 col-md-offset-3 col-lg-offset-3"> 
      <div class = "well well-lg content-well"> 
        <span class = "content" id = "content-span">	
	        <blockquote class = "message" id = "message">"`+msg+`"</blockquote> 
	        <caption class = "author" id = "author">"`+author+`"</caption> 
        </span> 
      </div> 
    </div> 
  </div>`

}*/



function sendComment(form) {
	var msg = form.message.value;
	var author = form.author.value;
	msg = msg.replace(",","\\\\,")
	author = author.replace(",","\\\\");
	if(msg == "" && author == "") window.location.reload(false);
	console.log(msg+" "+author);
	var request = new XMLHttpRequest();
	request.open('POST', "/add?author="+author+"&message="+msg, true);
	request.send();
	request.onload = function() {
	  if (request.status >= 200 && request.status < 400) {
	    // Success!
	    if(request.readyState == 4) {
	    	console.log(request.response);
	    	window.location.reload(false); 
	    }
	  } else {
	    // We reached our target server, but it returned an error

	  }
	};

}

function loadComments() {
	var request = new XMLHttpRequest();
	request.open('GET', '/comments', true);
	request.responseType = "json";
	request.onload = function() {
	  if (request.status >= 200 && request.status < 400) {
	    // Success!
	    if(request.readyState == 4) {
	    	console.log(request.response);
	    	comments = request.response;
	    	appendComments();
	    	
	    }
	  } else {
	    // We reached our target server, but it returned an error

	  }
	};

	request.onerror = function() {
	  // There was a connection error of some sort
	};

	request.send();
}

function appendComments() {
	var divComments = document.getElementById("commentsSection");
	comments.comments.reverse().forEach(function(comment) {
		comment.msg = comment.message.replace("\\\\",",");
		comment.author = comment.author.replace("\\\\",",");
		var commentBlock = document.getElementById("comment");
		var newCommentDiv = commentBlock.cloneNode(true);
		newCommentDiv.style = "";
		newCommentDiv.querySelector('#message').innerHTML = comment.message;
		newCommentDiv.querySelector('#author').innerHTML = comment.author;
		divComments.appendChild(newCommentDiv);
	});	

}
var tid = setInterval( function () {
    if ( document.readyState !== 'complete' ) return;
    clearInterval( tid );       
    loadComments();
}, 100 );