<?php
header("Document-Policy: oversized-images;scale=2.0");
?>
<!DOCTYPE html>
<body>
<img src="green-256x256.jpg" width="128" height="128" style="border: 10px solid red;">
<img src="green-256x256.jpg" width="120" height="120" style="border: 10px solid red;">
<img src="green-256x256.jpg" width="120" height="120" style="padding: 10px;">
<img src="green-256x256.jpg" width="120" height="120" style="border: 10px solid red; padding: 5px;">
</body>
