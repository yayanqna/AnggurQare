<?php
  include("connection.php");

  if(!empty($_POST)){
    $soil_ph = $_POST["soil_ph"];
    $lightness = $_POST["lightness"];
    $water_ph = $_POST["water_ph"];
    $water_temp = $_POST["water_temp"];
    $water_level = $_POST["water_level"];
    $nutrition_tds = $_POST["nutrition_tds"];
    $temperature = $_POST["temperature"];
    $humidity = $_POST["humidity"];
    $soil_moisture = $_POST["soil_moisture"];
    
    $query = "INSERT INTO log (soil_ph, lightness, water_ph, water_temp, water_level, nutition_tds, temperature, humidity, soil_moisture)
            VALUES ('".$soil_ph."','".$lightness."','".$water_ph."','".$water_temp."','".$water_level."','".$nutrition_tds."','".$temperature."','".$humidity."','".$soil_moisture."')";
    if ($conn->query($query) === TRUE) {
      echo "Berhasil menyimpan data ke table log";
    } else {
      echo "Error: " . $sql . "<br>" . $conn->error;
    }
  }

?>