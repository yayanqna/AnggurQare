<?php
  include("anggurqare_connection.php");

  if(!empty($_POST)){
    $soil_ph = $_POST["soil_ph"];
    $soil_moisture = $_POST["soil_moisture"];
    $nutrition_tds = $_POST["nutrition_tds"];
    $water_ph = $_POST["water_ph"];
    $water_temp = $_POST["water_temp"];
    $water_level = $_POST["water_level"];
    $temperature = $_POST["temperature"];
    $lightness = $_POST["lightness"];
    $humidity = $_POST["humidity"];
    
    $query = "INSERT INTO anggurqare_log (soil_ph, soil_moisture, nutition_tds, water_ph, water_temp, water_level, temperature, lightness, humidity)
            VALUES ('".$soil_ph."','".$soil_moisture."','".$nutrition_tds."','".$water_ph."','".$water_temp."','".$water_level."','".$temperature."','".$lightness."','".$humidity."')";
    if ($conn->query($query) === TRUE) {
      echo "Berhasil menyimpan data ke table log";
    } else {
      echo "Error: " . $sql . "<br>" . $conn->error;
    }
  }

?>