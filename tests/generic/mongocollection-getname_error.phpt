--TEST--
MongoCollection::getName()
--SKIPIF--
<?php require dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php
require dirname(__FILE__) . "/../utils.inc";
$m = mongo();

$c = $m->phpunit->col;
// testing an unneeded parameter
echo "Working with collection " . $c->getName('test') . ".\n";
?>
--EXPECT--
Working with collection col.
