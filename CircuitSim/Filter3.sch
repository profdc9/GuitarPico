<Qucs Schematic 1.0.0>
<Properties>
  <View=0,0,1150,800,1,0,0>
  <Grid=10,10,1>
  <DataSet=Filter3.dat>
  <DataDisplay=Filter3.dpl>
  <OpenDisplay=1>
  <Script=Filter3.m>
  <RunScript=0>
  <showFrame=0>
  <FrameText0=Title>
  <FrameText1=Drawn By:>
  <FrameText2=Date:>
  <FrameText3=Revision:>
</Properties>
<Symbol>
</Symbol>
<Components>
  <Vac V1 1 330 360 18 -26 0 1 "1 V" 1 "1 GHz" 0 "0" 0 "0" 0>
  <GND * 1 330 390 0 0 0 0>
  <R R1 1 430 250 -26 15 0 0 "10k Ohm" 1 "26.85" 0 "0.0" 0 "0.0" 0 "26.85" 0 "european" 0>
  <OpAmp OP1 1 610 230 -26 42 0 0 "1e6" 1 "15 V" 0>
  <R R2 1 520 250 -26 15 0 0 "10k Ohm" 1 "26.85" 0 "0.0" 0 "0.0" 0 "26.85" 0 "european" 0>
  <R R3 1 620 460 -26 15 0 0 "10k Ohm" 1 "26.85" 0 "0.0" 0 "0.0" 0 "26.85" 0 "european" 0>
  <GND * 1 430 130 0 0 0 0>
  <R R4 1 740 260 15 -26 0 1 "10k Ohm" 1 "26.85" 0 "0.0" 0 "0.0" 0 "26.85" 0 "european" 0>
  <GND * 1 740 320 0 0 0 0>
  <.AC AC1 1 60 40 0 40 0 0 "log" 1 "10 Hz" 1 "100 kHz" 1 "199" 1 "no" 0>
  <C C2 1 470 190 17 -26 0 1 "2200 pF" 1 "" 0 "neutral" 0>
  <C C1 1 620 360 -26 17 0 0 "470 pF" 1 "" 0 "neutral" 0>
</Components>
<Wires>
  <550 250 560 250 "" 0 0 0 "">
  <460 250 470 250 "" 0 0 0 "">
  <330 250 400 250 "" 0 0 0 "">
  <330 250 330 330 "" 0 0 0 "">
  <650 230 700 230 "" 0 0 0 "">
  <700 230 700 360 "" 0 0 0 "">
  <650 460 700 460 "" 0 0 0 "">
  <700 360 700 460 "" 0 0 0 "">
  <650 360 700 360 "" 0 0 0 "">
  <560 250 580 250 "" 0 0 0 "">
  <560 250 560 360 "" 0 0 0 "">
  <560 360 590 360 "" 0 0 0 "">
  <470 250 490 250 "" 0 0 0 "">
  <470 250 470 460 "" 0 0 0 "">
  <470 460 590 460 "" 0 0 0 "">
  <580 130 580 210 "" 0 0 0 "">
  <430 130 470 130 "" 0 0 0 "">
  <470 220 470 250 "" 0 0 0 "">
  <470 130 580 130 "" 0 0 0 "">
  <470 130 470 160 "" 0 0 0 "">
  <740 290 740 320 "" 0 0 0 "">
  <700 230 740 230 "" 0 0 0 "">
  <740 230 740 230 "OutputV" 770 200 0 "">
</Wires>
<Diagrams>
  <Rect 880 220 240 160 3 #c0c0c0 1 11 1 0 20000 100000 1 0.4 0.2 1.04949 1 -1 1 1 315 0 225 0 0 0 "" "" "">
	<"ngspice/ac.v(outputv)" #0000ff 0 3 0 0 0>
  </Rect>
</Diagrams>
<Paintings>
</Paintings>