<?php
header("Document-Policy: oversized-images;scale=2.0");
?>
<!DOCTYPE html>
<body>
  <img src="green-256x256.jpg" intrinsicsize="100 x 100" width="127" height="127">
  <img srcset="green-256x256.jpg 256w" sizes="100px" width="127" height="127">
  <img srcset="green-256x256.jpg 256w" sizes="100px" width="128" height="128">
  <img srcset="green-256x256.jpg 256w" sizes="100px" width="129" height="129">
</body>
