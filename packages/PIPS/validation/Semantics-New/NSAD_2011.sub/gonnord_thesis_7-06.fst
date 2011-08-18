
model gonnord_thesis_7_06 {

	var x, y, z;
	states k1, k2;

	transition t_ini := {
		from := k1;
		to := k2;
		guard := x = 0 && y = 0 && z = 0;
		action := ;
	};

	transition t1 := {
		from := k2;
		to := k2;
		guard := x >= 2 * z;
		action := x' = x + 1, y ' = y + 1, z' = z + 1;
	};

	transition t2 := {
		from := k2;
		to := k2;
		guard := true;
		action := z' = 0;
	};

}

strategy s {

	Region init := {state = k1};

}

