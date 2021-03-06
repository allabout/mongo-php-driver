--TEST--
Test for bug PHP-402: MongoCollection::validate(true) doesn't set the correct scan-all flag.
--DESCRIPTION--
This test skips mongos because its validate() results contain only a top-level
"validate" field, which collects the shard results, and grouped results for each
shard in a "raw" array field.
--SKIPIF--
<?php require dirname(__FILE__) ."/skipif.inc"; ?>
<?php require dirname(__FILE__) . '/skipif_mongos.inc'; ?>
--FILE--
<?php
require dirname(__FILE__) ."/../utils.inc";

$m = mongo();
$c = $m->selectCollection('phpunit', 'col');
$c->insert(array('x' => 1), array('safe' => true));

$result = $c->validate();
var_dump(isset($result['warning']));
var_dump($result['warning']);

$result = $c->validate(true);
var_dump(isset($result['warning']));

$c->drop();
var_dump($c->validate());;
?>
--EXPECT--
bool(true)
string(79) "Some checks omitted for speed. use {full:true} option to do more thorough scan."
bool(false)
array(2) {
  ["errmsg"]=>
  string(12) "ns not found"
  ["ok"]=>
  float(0)
}
