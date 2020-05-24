<?php
echo("Save Data\n");
if (array_key_exists("time", $_GET) && array_key_exists("value1", $_GET) && array_key_exists("value2", $_GET)) {
    $contents = $_GET["time"] . "; " . $_GET["value1"] . "; " . $_GET["value2"] . "\n"; 
    echo($contents);
    file_put_contents("./test.log", $contents, FILE_APPEND);
    echo("Data Saved/ run script");
    #system("./plot.py");
    $message = exec("./plot.py 2>&1");
    echo($message);
    
} else {
    echo("False Keys found");
}

?>
