<?php

$fp = fopen("test.log","a");

$obj = new \registerPM\Clasesilla();
$obj->setCleanMode(10);
$mode = $obj->getCleanMode();
$res = $obj->DoSomething($_SERVER['REQUEST_URI']);
fwrite($fp, $res . PHP_EOL);

$config = new \registerPM\Config\Adapter\Ini('config.ini');
fwrite($fp, print_r($config,true) . PHP_EOL);

$_config = array(
                "phalcon" => array(
                                "baseuri" => "/phalcon/"
                ),
                "models" => array(
                                "metadata" => "memory"
                ),
                "database" => array(
                                "adapter" => "mysql",
                                "host" => "localhost",
                                "username" => "user",
                                "password" => "passwd",
                                "name" => "demo"
                ),
                "test" => array(
                                "parent" => array(
                                                "property" => 1,
                                                "property2" => "yeah"
                                )
                )
);

$config2 = new registerPM\Config($_config);

class other extends \registerPM\Clasesilla
{
    public $m_ss;
    public function anotherFunction()
    {
        $thiz->m_ss = "do the test";
    }
};
fwrite($fp, "class other created" . PHP_EOL);

$x = new other();
$x->setCleanMode(55);
$mode1 = $x->getCleanMode();
fwrite($fp, "mode:". $mode1 . PHP_EOL);
$res1 = $x->DoSomething("any/string");
fwrite($fp, "res:". $res1 . PHP_EOL);
$x->anotherFunction();
fwrite($fp, "end..." . PHP_EOL);
fclose($fp);
