--TEST--
bson_encode() double
--SKIPIF--
<?php require dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php
$expected = chr(236).chr(81).chr(184).chr(30).chr(133).chr(235).chr(16).chr(64);
var_dump($expected === bson_encode(4.23));
?>
--EXPECT--
bool(true)
