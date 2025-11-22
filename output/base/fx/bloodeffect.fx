fx fx/collide_bloodsplat {
	{
		delay		0
		restart		0
		duration	2.5
		sound		"bloodsplat"
	}
}

fx fx/collide_bloodsplat_small {
	{
		delay		0
		restart		0
		duration	2.5
		sound		"monster_zombie_commando_tentacle_in"
	}
}

fx fx/collide_bloodsplat_medium {
	{
		delay		0
		restart		0
		duration	2.5
		sound		"monster_zombie_commando_tentacle_out"
	}
	{
		delay		0
		restart		0
		duration	7200
		offset		0, 0, 7
		particle	"flies3.prt"
	}
	{
		delay		0
		restart		0
		duration	7200
		offset		0, 0, 7
		sound		"sound/flies3"
	}
}

fx fx/collide_bloodsplat_hard {
	{
		delay		0
		restart		0
		duration	2.5
		sound		"trite_deathsplat"
	}
	{
		delay		0
		duration	0.5
		restart		0
		offset		0, 0, 24
		particle	"bloodwound.prt"
		trackorigin	1
	}
}

fx fx/fx_gib {
	//bindto body
	{
		delay		0
		name		"blood_effect"
		duration	4
		restart		0
		model		"bloodsplat.prt"
		offset		0,0,0
	}
	{
		delay		0
		restart		0
		duration	4
		offset		0, 0, 0
		sound		"sound/ed/tick/tick_burn"
	}
}