#include <cmath>

#include "EnemyInst.h"
#include "AnimatedInst.h"
#include "ProjectileInst.h"
#include "PlayerInst.h"

#include "../GameState.h"

#include "../../display/display.h"

#include "../../data/sprite_data.h"
#include "../../data/enemy_data.h"


#include "../../util/math_util.h"
#include "../../util/collision_util.h"

static const int DEPTH = 50;

EnemyInst::EnemyInst(int enemytype, int x, int y) :
	GameInst(x,y, game_enemy_data[enemytype].radius, true, DEPTH),
	eb(game_enemy_data[enemytype].basestats.movespeed),
	enemytype(enemytype), rx(x), ry(y),
	xpgain(game_enemy_data[enemytype].xpaward),
	stat(game_enemy_data[enemytype].basestats) {
	last_seen_counter = 0;
}

EnemyInst::~EnemyInst() {
}

EnemyType* EnemyInst::etype(){
	return &game_enemy_data[enemytype];
}


void EnemyInst::init(GameState* gs) {
	MonsterController& mc = gs->monster_controller();
	mc.register_enemy(this);

	//xpgain *=1+gs->branch_level()/10.0;
	//xpgain = round(xpgain/5.0)*5;
	int ln = gs->level()->level_number+1;
	stats().hp += stats().hp*ln/10.0;
	stats().max_hp += stats().max_hp*ln/10.0;
	stats().mp += stats().mp*ln/10.0;
	stats().max_mp += stats().max_mp*ln/10.0;
	stats().magicatk.cooldown /= 1.0 + ln/10.0;
	stats().magicatk.damage *= 1.0 + ln/10.0;
	stats().magicatk.projectile_speed *= 1.0 + ln/10.0;
	stats().meleeatk.cooldown /= 1.0 + ln/10.0;
	stats().meleeatk.damage *= 1.0 + ln/10.0;
	stats().max_mp += stats().max_mp*ln/10.0;
	double speedfactor = 1+stats().max_mp*ln/10.0;
	if (stats().movespeed < 3)
		eb.speed = std::min(stats().movespeed*speedfactor,3.0);
}

void EnemyInst::step(GameState* gs) {
	//Much of the monster implementation resides in MonsterController
	stats().step();
}
void EnemyInst::draw(GameState* gs) {
	GameView& view = gs->window_view();
	GLimage& img = game_sprite_data[etype()->sprite_number].img;

	int w = img.width, h = img.height;
	int xx = x - w / 2, yy = y - h / 2;

	if (!view.within_view(xx, yy, w, h))
		return;
	if (!gs->object_visible_test(this))
		return;

	if (stats().hp < stats().max_hp)
		gl_draw_statbar(view, x - 10, y - 20, 20, 5, stats().hp, stats().max_hp);

	if (stats().hurt_cooldown > 0){
		float s = 1 - stats().hurt_alpha();
		Colour red(255,255*s,255*s);
		gl_draw_image(&img, xx - view.x, yy - view.y, red);
	}
	else{
		gl_draw_image(&img, xx - view.x, yy - view.y);
//		if (gs->solid_test(this)){
//		Colour red(255,0,0);
//		image_display(&img, xx - view.x, yy - view.y,red);
//		}
	}
//	gl_printf(gs->primary_font(), Colour(255,255,255), x - view.x, y-25 -view.y, "id=%d", id);
	//draw_path(gs, eb.path);
}


void EnemyInst::attack(GameState* gs, GameInst* inst, bool ranged){
	if (stats().has_cooldown()) return;
	PlayerInst* pinst;
	if ( (pinst = dynamic_cast<PlayerInst*>(inst))) {
		if (ranged){
			Attack& ranged = stats().magicatk;

			Pos p(pinst->x, pinst->y);
			p.x += gs->rng().rand(-12,+13);
			p.y += gs->rng().rand(-12,+13);
			if (gs->tile_radius_test(p.x, p.y, 10)) {
				p.x = pinst->x;
				p.y = pinst->y;
			}
			GameInst* bullet = new ProjectileInst(id, ranged, x,y,p.x, p.y);
			gs->add_instance(bullet);
			stats().reset_ranged_cooldown();
			stats().cooldown += gs->rng().rand(-4,5);
		} else {
			pinst->stats().hurt(stats().meleeatk.damage);

			char dmgstr[32];
			snprintf(dmgstr, 32, "%d", stats().meleeatk.damage);
			float rx, ry;
			direction_towards(Pos(x,y), Pos(pinst->x, pinst->y), rx, ry, 0.5);
			gs->add_instance(new AnimatedInst(pinst->x-5 + rx*5,pinst->y+ry*5, -1, 25,
					rx, ry, dmgstr));

			stats().reset_melee_cooldown();
			stats().cooldown += gs->rng().rand(-4,5);
		}
	}
}


bool EnemyInst::hurt(GameState* gs, int hp){
	if (!destroyed && stats().hurt(hp)){
		gs->add_instance(new AnimatedInst(x,y,etype()->sprite_number, 15));
		gs->monster_controller().deregister_enemy(this);
		gs->remove_instance(this);
		return true;
	}
	return false;
}
