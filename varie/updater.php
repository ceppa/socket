<?
error_reporting(E_ALL);
$output="";
$inizio_estate="0321";
$inizio_inverno="0921";


if(!isset($_GET["op"]))
	$_GET["op"]="meteo";


switch($_GET["op"])
{
	case "ultimoavviso":
		include("mysql.php");
		$conn=mysql_connect($address,$myuser,$mypass)
			or die("Connessione non riuscita".mysql_error());
		mysql_select_db($mydb,$conn)
			or die("Connessione non riuscita".mysql_error());

		$query="SELECT analog.id_analog,
						alarm_ack.data,
						analog.description as analog_desc,
						digital.description as digital_desc,
						digital.id_digital,
						alarm_ack.msg 
					FROM alarm_ack
							LEFT JOIN analog
								ON alarm_ack.id_analog=analog.id_analog
							LEFT JOIN digital
								ON alarm_ack.id_digital=digital.id_digital
					ORDER BY data desc
					LIMIT 0,1";
		$result=mysql_query($query)
			or die("$query<br>".mysql_error());
		$data_alarm=0;
		if(mysql_num_rows($result))
		{
			$row=mysql_fetch_assoc($result);
			$data_alarm=strtotime($row['data']);
	
			$ultimoavviso=date("d.m.Y H:i",$data_alarm)." - ";
			if(strlen($row["id_analog"]))
				$ultimoavviso.=$row["analog_desc"];
			elseif(strlen($row["id_digital"]))
				$ultimoavviso.=$row["digital_desc"];
			$ultimoavviso.=" - ".$row["msg"];
			mysql_free_result($result);
		}
		else
			$ultimoavviso="";

		$query="SELECT data,event_type,device_num,ch_num,ch_id,msg
				FROM events
				ORDER BY id DESC
				LIMIT 0,1";
		$result=mysql_query($query)
			or die("$query<br>".mysql_error());
		if(mysql_num_rows($result))
		{
			$row=mysql_fetch_assoc($result);
			$data_event=strtotime($row['data']);
			if($data_event>$data_alarm)
			{
				$ultimoavviso=date("d.m.Y H:i",$data_event)." - ";
				if($row["event_type"]==1)
					$ultimoavviso.="Irrigazione ".$row["device_num"];
				elseif($row["event_type"]==2)
					$ultimoavviso.="Irrigazione ".$row["device_num"]." - Circuito ".$row["ch_num"];
				$ultimoavviso.=" - ".$row["msg"];
			}
			mysql_free_result($result);
		}
	
		mysql_close($conn);
		$output="ultimoavviso=$ultimoavviso";
		echo $output;
		die();
		break;
	case "effemeridi":
		require_once("socket_communication.php");
		$stringa=sendToSocket("get effemeridi");
		$exploded=explode(" ",$stringa);
		$dawn=$exploded[0];
		$sunset=$exploded[1];
		$output="alba=$dawn&tramonto=$sunset";
		echo $output;
		die();
		break;
	case "sottostato":
		require_once("socket_communication.php");
		$stringa=sendToSocket("get sottostato $inizio_estate $inizio_inverno");
		$exploded=explode(" ",$stringa);
		$antiintrusione=($exploded[0]>0?"abilitato":"disabilitato");
		$pioggia=($exploded[1]>0?"pioggia":"nopioggia");
		$giorno_notte=($exploded[2]>0?"giorno":"notte");
		$estate_inverno=($exploded[3]>0?"estate":"inverno");
		$output="giorno_notte=$giorno_notte&abilitato_disabilitato=$antiintrusione";
		$output.="&pioggia_nopioggia=$pioggia&estate_inverno=$estate_inverno";
		echo $output;
		die();
		break;
	default:
		require_once("socket_communication.php");
		$stringa=sendToSocket("get latest_values");
		$exploded=explode("`",$stringa);
		$valori=array();
		foreach($exploded as $linea)
		{
			if(strlen(trim($linea)))
			{
				$exp=explode(" ",$linea);
				$valori[$exp[0]][$exp[1]][$exp[2]]=$exp[3];
			}
		}
		$values_op=$_GET["op"];
		switch($values_op)
		{
			case "meteo":
				$ventod_value=(double)$valori[0][2][1];
				if(($ventod_value < 22.5) || ($ventod_value > 337.5))
					$ventod="Nord";
				elseif($ventod_value < 67.5)
					$ventod="Nord Est";
				elseif($ventod_value < 112.5)
					$ventod="Est";
				elseif($ventod_value < 157.5)
					$ventod="Sud Est";
				elseif($ventod_value < 202.5)
					$ventod="Sud";
				elseif($ventod_value < 247.5)
					$ventod="Sud Ovest";
				elseif($ventod_value < 292.5)
					$ventod="Ovest";
				else
					$ventod="Nord Ovest";

				$ventoi_value=(double)$valori[0][2][2]; 
				if($ventoi_value < 0.3)
					{
					$ventoi="Calma";
					$ventod="----";
					}
				elseif($ventoi_value < 1.6)
					$ventoi="Bava di vento";
				elseif($ventoi_value < 3.4)
					$ventoi="Brezza leggera";
				elseif($ventoi_value < 5.5)
					$ventoi="Brezza tesa";
				elseif($ventoi_value < 8.0)
					$ventoi="Vento moderato";
				elseif($ventoi_value < 10.8)
					$ventoi="Vento teso";
				elseif($ventoi_value < 13.9)
					$ventoi="Vento fresco";
				elseif($ventoi_value < 17.2)
					$ventoi="Vento forte";
				elseif($ventoi_value < 20.8)
					$ventoi="Burrasca";
				elseif($ventoi_value < 24.5)
					$ventoi="Burrasca forte";
				elseif($ventoi_value < 28.8)
					$ventoi="Tempesta";
				elseif($ventoi_value < 32.7)
					$ventoi="Tempesta violenta";
				else
					$ventoi="Uragano";

				if(isset($valori[0][2]))
				{
					$umidita=sprintf("%.0f",$valori[0][2][4]);
					$pressione=sprintf("%.0f",$valori[0][2][3]);
				}
				else
				{
					$umidita="---";
					$pressione="---";
				}

				if(isset($valori[0][1]))
					$temperatura=sprintf("%.1f",$valori[0][1][1]);
				else
					$temperatura="---";


				$output.="temperatura=$temperatura&pressione=$pressione&umidita=$umidita";
				$output.="&ventoi=$ventoi&ventod=$ventod";
				echo $output;
				break;
			case "riscaldamento":
				if((isset($valori[0][2]))&&(isset($valori[0][2])))
				{
					$rta=sprintf("%.1f",$valori[0][2][8]);
					$rtb=sprintf("%.1f",$valori[0][2][9]);
					$rtc=sprintf("%.1f",$valori[0][2][11]);
					$rtd=sprintf("%.1f",$valori[0][2][13]);
					$rte=sprintf("%.1f",$valori[0][2][6]);
					$rtf=sprintf("%.1f",$valori[0][2][10]);
					$rtg=sprintf("%.1f",$valori[0][2][12]);
					$rth=sprintf("%.1f",$valori[0][2][14]);
					$r1=($valori[1][1][1]?"1":"0");
					$r2=($valori[1][1][2]?"1":"0");
					$r3=($valori[1][1][1]?"1":"0");
					$r4=($valori[1][1][2]?"1":"0");
					$r5=($valori[1][1][1]?"1":"0");
					$r6=($valori[1][1][2]?"1":"0");
				}
				else
				{
					$rta="---";
					$rtb="---";
					$rtc="---";
					$rtd="---";
					$rte="---";
					$rtf="---";
					$rtg="---";
					$rth="---";
					$r1=0;
					$r2=0;
					$r3=0;
					$r4=0;
					$r5=0;
					$r6=0;
				}
				$output.="rta=$rta&rtb=$rtb&rtc=$rtc&rtd=$rtd&rte=$rte&rtf=$rtf&rtg=$rtg&rth=$rth";
				$output.="&r1=$r1&r2=$r2&r3=$r3&r4=$r4&r5=$r5&r6=$r6";
				echo $output;
				break;
			case "geotermico":
				if((isset($valori[0][2]))&&(isset($valori[0][2])))
				{
					$ggtc=sprintf("%.1f",$valori[0][2][8]);
					$ggtb=sprintf("%.1f",$valori[0][2][7]);
					$ggtd=sprintf("%.1f",$valori[0][2][6]);
					$ggta=sprintf("%.1f",$valori[0][2][15]);
					$ggte=sprintf("%.1f",$valori[0][2][16]);
					$ggtg=sprintf("%.1f",$valori[0][2][17]);
					$ggtf=sprintf("%.1f",$valori[0][2][18]);
				}
				else
				{
					$ggta="---";
					$ggtb="---";
					$ggtc="---";
					$ggtd="---";
					$ggte="---";
					$ggtf="---";
					$ggtg="---";			
				}
				$output.="gm=1&ggta=$ggta&ggtb=$ggtb&ggtc=$ggtc&ggtd=$ggtd&ggte=$ggte&ggtf=$ggtf&ggtg=$ggtg";
				echo $output;
				break;
			case "elettrico":
				if((isset($valori[3][1])))
				{
					$egt1=sprintf("%.1f",$valori[3][1][1]);
					$egt2=sprintf("%.1f",$valori[3][1][2]);
					$egt3=sprintf("%.1f",$valori[3][1][3]);
					$egt1=230;
					$egt2=10;
					$egt3=1;
				}
				else
				{
					$egt1="---";
					$egt2="---";
					$egt3="---";
				}
				if((isset($valori[1][1])))
				{
					$egu3=($valori[1][1][1]?"1":"0");
					$egu2=($valori[1][1][2]?"1":"0");
					$egu1=($valori[1][1][1]?"1":"0");
					$egu=($valori[1][1][2]?"1":"0");
					$ego3=($valori[1][1][1]?"1":"0");
					$ego2=($valori[1][1][2]?"1":"0");
					$ego1=($valori[1][1][1]?"1":"0");
					$ego=($valori[1][1][1]?"1":"0");
					$egl4=($valori[1][1][2]?"1":"0");
					$egl3=($valori[1][1][1]?"1":"0");
					$egl2=($valori[1][1][2]?"1":"0");
					$egl1=($valori[1][1][1]?"1":"0");
					$egl=($valori[1][1][2]?"1":"0");
					$ege=($valori[1][1][1]?"1":"0");
					$egp7=($valori[1][1][2]?"1":"0");
					$egp6=($valori[1][1][1]?"1":"0");
					$egp5=($valori[1][1][2]?"1":"0");
					$egp4=($valori[1][1][1]?"1":"0");
					$egp3=($valori[1][1][2]?"1":"0");
					$egp2=($valori[1][1][1]?"1":"0");
					$egp1=($valori[1][1][2]?"1":"0");
					$egb3=($valori[1][1][2]?"1":"0");
					$egb2=($valori[1][1][1]?"1":"0");
					$egb1=($valori[1][1][2]?"1":"0");
					$egbd=($valori[1][1][1]?"1":"0");
					$egq7=($valori[1][1][1]?"1":"0");
					$egq6=($valori[1][1][2]?"1":"0");
					$egq5=($valori[1][1][1]?"1":"0");
					$egq4=($valori[1][1][2]?"1":"0");
					$egq3=($valori[1][1][1]?"1":"0");
					$egq2=($valori[1][1][2]?"1":"0");
					$egqd=($valori[1][1][2]?"1":"0");
					$egq=($valori[1][1][1]?"1":"0");
					$eg=($valori[1][1][2]?"1":"0");
					$egq1=($valori[1][1][1]?"1":"0");
					$egb=($valori[1][1][2]?"1":"0");
					$egp=($valori[1][1][1]?"1":"0");
					$egs=($valori[1][1][2]?"1":"0");			
				}
				else
				{
					$egu3=0;
					$egu2=0;
					$egu1=0;
					$egu=0;
					$ego3=0;
					$ego2=0;
					$ego1=0;
					$ego=0;
					$egl4=0;
					$egl3=0;
					$egl2=0;
					$egl1=0;
					$egl=0;
					$ege=0;
					$egp7=0;
					$egp6=0;
					$egp5=0;
					$egp4=0;
					$egp3=0;
					$egp2=0;
					$egp1=0;
					$egb3=0;
					$egb2=0;
					$egb1=0;
					$egbd=0;
					$egq7=0;
					$egq6=0;
					$egq5=0;
					$egq4=0;
					$egq3=0;
					$egq2=0;
					$egqd=0;
					$egq=0;
					$eg=0;	
					$egq1=0;
					$egb=0;
					$egp=0;
					$egs=0;						
				}
				$output.="egt1=$egt1&egt2=$egt2&egt3=$egt3";
				$output.="&egu3=$egu3&egu2=$egu2&egu1=$egu1&egu=$egu&ego3=$ego3&ego2=$ego2&ego1=$ego1&ego=$ego&egl4=$egl4&egl3=$egl3&egl2=$egl2&egl1=$egl1&egl=$egl&ege=$ege&egp7=$egp7&egp6=$egp6&egp5=$egp5&egp4=$egp4&egp3=$egp3&egp2=$egp2&egp1=$egp1&egb3=$egb3&egb2=$egb2&egb1=$egb1&egbd=$egbd&egq7=$egq7&egq6=$egq6&egq5=$egq5&egq4=$egq4&egq3=$egq3&egq2=$egq2&egqd=$egqd&egq=$egq&eg=$eg&egq1=$egq1&egb=$egb&egp=$egp&egs=$egs";
				echo $output;
				break;
			case "irrigazione":
				if((isset($valori[0][2]))&&(isset($valori[0][2])))
				{
					$rta=sprintf("%.1f",$valori[0][2][8]);
					$rtb=sprintf("%.1f",$valori[0][2][7]);
					$ira=($valori[1][1][1]?"1":"0");
					$irb=($valori[1][1][2]?"1":"0");
					$irc=($valori[1][1][1]?"1":"0");
					$ird=($valori[1][1][2]?"1":"0");
					$ire=($valori[1][1][1]?"1":"0");
					$irf=($valori[1][1][2]?"1":"0");
				}
				else
				{
					$rtaa="---";
					$rtbb="---";
					$ira=-1;
					$irb=-1;
					$irc=-1;
					$ird=-1;
					$ire=-1;
					$irf=-1;
				}
				$output.="rta=$rta&rtb=$rtb&ira=$ira&irb=$irb&irc=$irc&ird=$ird&ire=$ire&irf=$irf";
				echo $output;
				break;
			default:
				$output.="&spentascale_accesascale=spentascale&noallarme_siallarme=noallarme";
				$output.="&noallarmeacqua_siallarmeacqua=noallarmeacqua";
				$output.="&noallarmegas_siallarmegas=noallarmegas&tempcentraletermica=19";
				$output.="&noallarmeacquact_siallarmeacquact=noallarmeacquact&tempgarage=23";
				$output.="&noallarmeacquagarage_siallarmeacquagarage=noallarmeacquagarage";
				$output.="&noallarmeintrusionegar_siallarmeintrusionegar=noallarmeintrusionegar";
				$output.="&tempmagazzino=19&spentamagazzino_accesamagazzino=spentamagazzino";
				$output.="&spentafitnessroom_accesafitnessroom=spentafitnessroom";
				$output.="&tempfitness=18&spentaterrazza_accesaterrazza=accesaterrazza";
				$output.="&temptaverna=20&noallarmemctaverna_siallarmemctaverna=noallarmemctaverna";
				$output.="&noallarmeintrusionetav_siallarmeintrusionetav=noallarmeintrusionetav";
				$output.="&spentataverna_accesataverna=acceasataverna";
				$output.="&preseestdisterrazza_preseestabilterrazza=preseestdisterrazza";
				$output.="&noallarmeincendioct_siallarmeincendioct=noallarmeincendioct";
				$output.="&tempstudio=21&cancellochiuso_cancelloaperto=cancelloaperto";
				$output.="&portonechiuso_portoneaperto=portoneaperto";
				$output.="&orologioanteriore_manualeanteriore=orologioanteriore";
				$output.="&orologioposteriore_manualeposteriore=manualeposteriore";
				$output.="&spenteposteriore_acceseposteriore=spenteposteriore";
				$output.="&spenteanteriore_acceseanteriore=acceseanteriore";
				$output.="&circuito1nonattivoant_circuito1attivoant=circuito1nonattivoant";
				$output.="&circuito2nonattivoant_circuito2attivoant=circuito2nonattivoant";
				$output.="&circuito1nonattivopost_circuito1attivopost=circuito1nonattivopost";
				$output.="&circuito2nonattivopost_circuito2attivopost=circuito2nonattivopost";
				$output.="&notermcantina_sitermcantina=sitermcantina";
				$output.="&anocircuitocantina_asicircuitocantina=asicircuitocantina";
				$output.="&bnocircuitocantina_bsicircuitocantina=bnocircuitocantina";
				$output.="&cnocircuitocantina_csicircuitocantina=csicircuitocantina";
				$output.="&anocircuitotaverna_asicircuitotaverna=asicircuitotaverna";
				$output.="&bnocircuitotaverna_bsicircuitotaverna=bnocircuitotaverna";
				$output.="&cnocircuitotaverna_csicircuitotaverna=csicircuitotaverna";
				$output.="&notermpt_sitermpt=sitermpt&notermct_sitermct=notermct";
				$output.="&anocircuitofitness_asicircuitofitness=asicircuitofitness";
				$output.="&bnocircuitofitness_bsicircuitofitness=bnocircuitofitness";
				$output.="&nocircuitobagno_sicircuitobagno=nocircuitobagno";
				$output.="&anocircuitovanotec_asicircuitovanotec=asicircuitovanotec";
				$output.="&bnocircuitovanotec_bsicircuitovanotec=bsicircuitovanotec";
				echo $output;
				break;
		}
		break;
}
?>
