import pcbnew
# import importlib
# importlib.reload(set_coords)

def set_coords():

	mm_scale = 15
	x_shift = 9
	y_shift = 0.0

	coords = [ ("1",   0.000, 13.856),
	           ("2",  -1.000, 13.856),
	           ("3",  -2.000, 13.856),
	           ("4",  -3.000, 13.856),
	           ("5",  -4.000, 13.856),
	           ("6",  -3.500, 12.990),
	           ("7",  -3.000, 12.124),
	           ("8",  -2.500, 12.990),
	           ("9",  -1.500, 12.990),
	           ("10", -2.000, 12.124),
	           ("11", -2.500, 11.258),
	           ("12", -2.000, 10.392),
	           ("13", -2.500, 9.526 ),
	           ("14", -2.000, 8.660 ),
	           ("15", -1.500, 7.794 ),
	           ("16", -1.000, 6.928 ),
	           ("17", -0.500, 6.062 ),
	           ("18",  0.000, 6.928 ),
	           ("19", -0.500, 7.794 ),
	           ("20", -1.000, 8.660 ),
	           ("21", -1.500, 9.526 ),
	           ("22", -0.500, 9.526 ),
	           ("23",  0.000, 8.660 ),
	           ("24",  0.500, 7.794 ),
	           ("25",  1.000, 8.660 ),
	           ("26",  0.500, 9.526 ),
	           ("27",  1.500, 9.526 ),
	           ("28",  1.000, 10.392),
	           ("29",  0.000, 10.392),
	           ("30", -1.000, 10.392),
	           ("31", -1.500, 11.258),
	           ("32", -0.500, 11.258),
	           ("33", -1.000, 12.124),
	           ("34", -0.500, 12.990),
	           ("35",  0.000, 12.124),
	           ("36",  0.500, 11.258),
	           ("37",  1.500, 11.258),
	           ("38",  1.000, 12.124),
	           ("39",  0.500, 12.990),
	           ("40",  1.000, 13.856),
	           ("41",  1.500, 12.990),
	           ("42",  2.000, 13.856),
	           ("43",  3.000, 13.856),
	           ("44",  4.000, 13.856),
	           ("45",  3.500, 12.990),
	           ("46",  2.500, 12.990),
	           ("47",  2.000, 12.124),
	           ("48",  3.000, 12.124),
	           ("49",  2.500, 11.258),
	           ("50",  2.000, 10.392),
	           ("51",  2.500, 9.526 ),
	           ("52",  2.000, 8.660 ),
	           ("53",  1.500, 7.794 ),
	           ("54",  1.000, 6.928 ),
	           ("55",  0.500, 6.062 ),
	           ("56",  1.000, 5.196 ),
	           ("57",  0.500, 4.330 ),
	           ("58",  0.000, 5.196 ),
	           ("59", -1.000, 5.196 ),
	           ("60", -0.500, 4.330 ),
	           ("61",  0.000, 3.464 ),
	           ("62", -0.500, 2.598 ),
	           ("63",  0.000, 1.732 ),
	           ("64",  0.500, 2.598 )]
	
	#print(coords)
	
	board = pcbnew.GetBoard()
	
	# update coords of LEDs and Cs
	for (refstr, x, y) in coords:
		refc = 'C'+refstr
		refl = 'LED'+refstr
	
		cfp = board.FindFootprintByReference(refc)
		if cfp:
			newx = pcbnew.FromMM((x+x_shift)*mm_scale)
			newy = pcbnew.FromMM((y+y_shift)*mm_scale)
			new_pos = pcbnew.VECTOR2I(newx, newy)
			cfp.SetPosition(new_pos)

		lfp = board.FindFootprintByReference(refl)
		if lfp:
			newx = (x+x_shift)*mm_scale*pcbnew.PCB_IU_PER_MM
			newy = (y+y_shift)*mm_scale*pcbnew.PCB_IU_PER_MM
			new_pos = pcbnew.VECTOR2I(int(newx), int(newy))
			lfp.SetPosition(new_pos)

	# draw edgecut
	edgecut = [ (-5.5 , 14.722),
	            (-3.0 , 10.392),
	            (-4.0 , 10.392),
	            (-1.5 ,  6.062),
	            (-2.5 ,  6.062),
	            (-1.0 ,  3.464),
	            (-2.0 ,  3.464),
	            ( 0.0 ,  0.0  ),
	            ( 2.0 ,  3.464),
	            ( 1.0 ,  3.464),
	            ( 2.5 ,  6.062),
	            ( 1.5 ,  6.062),
	            ( 4.0 , 10.392),
	            ( 3.0 , 10.392),
	            ( 5.5 , 14.722)]
	# add 7x7 cm foot
	edgecut = edgecut + [ ( 35.0/mm_scale, 14.722) ]
	edgecut = edgecut + [ ( 35.0/mm_scale, 14.722+70.0/mm_scale) ]
	edgecut = edgecut + [ (-35.0/mm_scale, 14.722+70.0/mm_scale) ]
	edgecut = edgecut + [ (-35.0/mm_scale, 14.722) ]

	edgepts = [(pcbnew.FromMM((x+x_shift)*mm_scale), pcbnew.FromMM((y+y_shift)*mm_scale)) for (x,y) in edgecut]

	sps = pcbnew.SHAPE_POLY_SET()
	chain = pcbnew.SHAPE_LINE_CHAIN()
	for (x,y) in edgepts:
		chain.Append(x,y)
	chain.SetClosed(True)
	sps.AddOutline(chain)

	ps = pcbnew.PCB_SHAPE(board, pcbnew.SHAPE_T_POLY)
	ps.SetPolyShape(sps)
	ps.SetFilled(False)
	ps.SetLayer(pcbnew.Edge_Cuts)

	board.Add(ps)
	
	pcbnew.Refresh()

