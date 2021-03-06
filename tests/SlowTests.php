<?php
require_once 'PHPUnit/Framework.php';

/**
 * Test class for MongoCursor.
 * Generated by PHPUnit on 2009-04-09 at 18:09:02.
 */
class SlowTests extends PHPUnit_Framework_TestCase
{
    /**
     * @var    MongoCursor
     * @access protected
     */
    protected $object;

    /**
     * Sets up the fixture, for example, opens a network connection.
     * This method is called before a test is executed.
     *
     * @access protected
     */
    protected function setUp()
    {
        $m = new Mongo();
        $this->object = $m->selectCollection('phpunit', 'c');
        $this->object->drop();
    }

    /**
     * @expectedException MongoCursorTimeoutException
     */
    public function testStaticTimeout() {
      $this->markTestSkipped("for now");
      return;

      MongoCursor::$timeout = 1;

      for ($i=0; $i<1000; $i++) {
        $this->object->insert(array("x" => "sdfjnaireojaerkgmdfkngkdsflngklsgntoigneorisgmsrklgd$i", "y" => $i));
      }

      $rows = $this->object->find(array('$eval' => 'r = 0; cursor = db.c.find(); while (cursor.hasNext()) { x = cursor.next(); for (i=0; i<200; i++) { if (x.name == "joe"+i) { r++; } } } return r;'));
      foreach ($rows as $row);

      MongoCursor::$timeout = 30000;
    }

    /**
     * @expectedException MongoCursorTimeoutException
     */
    public function testTimeout() {
      $cmd = $this->object->db->selectCollection('$cmd');

      for ($i=0; $i<10000; $i++) {
        $this->object->insert(array("name" => "joe".$i, "interests" => array(rand(), rand(), rand())));
      }

      // should time out
      $query = 'r = 0; cursor = db.c.find(); while (cursor.hasNext()) { x = cursor.next(); for (i=0; i<200; i++) { if (x.name == "joe"+i) { r++; } } } return r;';
      $cursor = $cmd->find(array('$eval'  => $query))->limit(-1)->timeout(2000);
      $this->assertNull($cursor->getNext());
    }

    public function testTimeout3() {
      for ($i=0; $i<10000; $i++) {
        $this->object->insert(array("name" => "joe".$i, "interests" => array(rand(), rand(), rand())));
      }

      $cmd = $this->object->db->selectCollection('$cmd');

      $query = 'r = 0; cursor = db.c.find(); while (cursor.hasNext()) { x = cursor.next(); for (i=0; i<200; i++) { if (x.name == "joe"+i) { r++; } } } return r;';
      $count = 0;
      for ($i=0; $i<3; $i++) {
        $cursor = $cmd->find(array('$eval'  => $query))->limit(-1)->timeout(500);

        try {
          $x = $cursor->getNext();
          $this->assertFalse(true, json_encode($x));
        }
        catch(MongoCursorTimeoutException $e) {
          $count++;
        }
      }

      $this->assertEquals(3, $count);
      $x = $this->object->findOne();
      $this->assertNotNull($x);
      $this->assertTrue(array_key_exists('name', $x), json_encode($x));
      $this->assertTrue(array_key_exists('interests', $x), json_encode($x));
    }
}

?>
