// Archive of SDB formats.

#ifndef WOD_SDB_H_
#define WOD_SDB_H_

#include "common.h"
#include "db.h"

VEC_DBType sdb_3713(Arena *arena) {
    VEC_DBType db = VEC_EMPTY;
    {
        DBType ty = {
            .TYPE_ID = 0,
            .TYPENAME = SV("マップ設定"),
            .DATANAME_LOAD_TYPE = 0,
            .DATANAME_LOAD_NAME = SV(""),
            .MEMO = SV("各IDで読み込みたい<<\\n>>マップファイル名と、<<\\n>>BGM・BGSを設定します。<<\\n>><<\\n>>「マップ新規作成」あるいは<<\\n>>「マップ設定」で設定した内容が<<\\n>>ここに反映されています。<<\\n>><<\\n>>各マップID名は、<<\\n>>[マップ選択]ウィンドウ内に<<\\n>>表示されます。"),
            .itemdef = VEC_EMPTY,
            .data = VEC_EMPTY,
        };
        {
            DBItemDef def = {
                .type = 2000,
                .ITEMNAME = SV("マップファイル名"),
                .VMEMO = SV(""),
                .LOADTYPE = 1,
                .CHOICE_NAME = VEC_EMPTY,
                .CHOICE_VAL = VEC_EMPTY,
                .DEFAULT_VAL = 0,
            };
            VEC_PUSH(def.CHOICE_NAME, SV("MapData"), arena);
            VEC_PUSH(def.CHOICE_VAL, 0, arena);
            VEC_PUSH(ty.itemdef, def, arena);
        }
        {
            DBItemDef def = {
                .type = 1000,
                .ITEMNAME = SV("BGM番号(ﾘｽﾄ読込)"),
                .VMEMO = SV(""),
                .LOADTYPE = 2,
                .CHOICE_NAME = VEC_EMPTY,
                .CHOICE_VAL = VEC_EMPTY,
                .DEFAULT_VAL = -1,
            };
            VEC_PUSH(def.CHOICE_NAME, SV("無し"), arena);
            VEC_PUSH(def.CHOICE_NAME, SV("イベントに任せる"), arena);
            VEC_PUSH(def.CHOICE_NAME, SV("ﾌｧｲﾙ名で指定"), arena);
            VEC_PUSH(def.CHOICE_VAL, 0, arena);
            VEC_PUSH(def.CHOICE_VAL, 1, arena);
            VEC_PUSH(def.CHOICE_VAL, 1, arena);
            VEC_PUSH(ty.itemdef, def, arena);
        }
        {
            DBItemDef def = {
                .type = 1000,
                .ITEMNAME = SV("BGS番号(ﾘｽﾄ読込)"),
                .VMEMO = SV(""),
                .LOADTYPE = 2,
                .CHOICE_NAME = VEC_EMPTY,
                .CHOICE_VAL = VEC_EMPTY,
                .DEFAULT_VAL = -1,
            };
            VEC_PUSH(def.CHOICE_NAME, SV("無し"), arena);
            VEC_PUSH(def.CHOICE_NAME, SV("イベントに任せる"), arena);
            VEC_PUSH(def.CHOICE_NAME, SV("ﾌｧｲﾙ名で指定"), arena);
            VEC_PUSH(def.CHOICE_VAL, 0, arena);
            VEC_PUSH(def.CHOICE_VAL, 2, arena);
            VEC_PUSH(def.CHOICE_VAL, 1, arena);
            VEC_PUSH(ty.itemdef, def, arena);
        }
        {
            DBItemDef def = {
                .type = 1000,
                .ITEMNAME = SV("ﾙｰﾌﾟ:1横2縦3両方"),
                .VMEMO = SV(""),
                .LOADTYPE = 3,
                .CHOICE_NAME = VEC_EMPTY,
                .CHOICE_VAL = VEC_EMPTY,
                .DEFAULT_VAL = 0,
            };
            VEC_PUSH(def.CHOICE_NAME, SV("ループ無し"), arena);
            VEC_PUSH(def.CHOICE_NAME, SV("横のみループ"), arena);
            VEC_PUSH(def.CHOICE_NAME, SV("縦のみループ"), arena);
            VEC_PUSH(def.CHOICE_NAME, SV("縦横ループ"), arena);
            VEC_PUSH(def.CHOICE_VAL, 0, arena);
            VEC_PUSH(def.CHOICE_VAL, 1, arena);
            VEC_PUSH(def.CHOICE_VAL, 2, arena);
            VEC_PUSH(def.CHOICE_VAL, 3, arena);
            VEC_PUSH(ty.itemdef, def, arena);
        }
        {
            DBItemDef def = {
                .type = 1000,
                .ITEMNAME = SV("遠景番号"),
                .VMEMO = SV(""),
                .LOADTYPE = 2,
                .CHOICE_NAME = VEC_EMPTY,
                .CHOICE_VAL = VEC_EMPTY,
                .DEFAULT_VAL = -2,
            };
            VEC_PUSH(def.CHOICE_NAME, SV("<なし>"), arena);
            VEC_PUSH(def.CHOICE_NAME, SV("イベントに任せる"), arena);
            VEC_PUSH(def.CHOICE_NAME, SV("ﾌｧｲﾙ名で指定"), arena);
            VEC_PUSH(def.CHOICE_VAL, 0, arena);
            VEC_PUSH(def.CHOICE_VAL, 13, arena);
            VEC_PUSH(def.CHOICE_VAL, 1, arena);
            VEC_PUSH(ty.itemdef, def, arena);
        }
        {
            DBItemDef def = {
                .type = 2000,
                .ITEMNAME = SV("BGMﾌｧｲﾙ名[BGM番号=-3]"),
                .VMEMO = SV(""),
                .LOADTYPE = 1,
                .CHOICE_NAME = VEC_EMPTY,
                .CHOICE_VAL = VEC_EMPTY,
                .DEFAULT_VAL = 0,
            };
            VEC_PUSH(def.CHOICE_NAME, SV("BGM"), arena);
            VEC_PUSH(def.CHOICE_VAL, 0, arena);
            VEC_PUSH(ty.itemdef, def, arena);
        }
        {
            DBItemDef def = {
                .type = 2000,
                .ITEMNAME = SV("BGSﾌｧｲﾙ名[BGS番号=-3]"),
                .VMEMO = SV(""),
                .LOADTYPE = 1,
                .CHOICE_NAME = VEC_EMPTY,
                .CHOICE_VAL = VEC_EMPTY,
                .DEFAULT_VAL = 0,
            };
            VEC_PUSH(def.CHOICE_NAME, SV("BGM"), arena);
            VEC_PUSH(def.CHOICE_VAL, 0, arena);
            VEC_PUSH(ty.itemdef, def, arena);
        }
        {
            DBItemDef def = {
                .type = 2000,
                .ITEMNAME = SV("遠景ﾌｧｲﾙ名[遠景番号=-3]"),
                .VMEMO = SV(""),
                .LOADTYPE = 1,
                .CHOICE_NAME = VEC_EMPTY,
                .CHOICE_VAL = VEC_EMPTY,
                .DEFAULT_VAL = 0,
            };
            VEC_PUSH(def.CHOICE_NAME, SV("Fog_BackGround"), arena);
            VEC_PUSH(def.CHOICE_VAL, 0, arena);
            VEC_PUSH(ty.itemdef, def, arena);
        }
        {
            DBData data = {
                .name = SV("サンプルマップ"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("MapData/SampleMap.mps")}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = -1}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = -1}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = -2}, arena);
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        VEC_PUSH(db, ty, arena);
    }
    {
        DBType ty = {
            .TYPE_ID = 1,
            .TYPENAME = SV("BGMリスト"),
            .DATANAME_LOAD_TYPE = 0,
            .DATANAME_LOAD_NAME = SV(""),
            .MEMO = SV("[サウンド]コマンド内の<<\\n>>[ｼｽﾃﾑDBから直接選択]で<<\\n>>選べるBGMをここで指定します。<<\\n>><<\\n>>ﾙｰﾌﾟ開始位置/ｷｰ±は、<<\\n>>○oggやMP3なら<<\\n>>　ﾙｰﾌﾟ開始位置(ms)<<\\n>>○MIDIならキーの高低<<\\n>>　（-24～+24）<<\\n>>となります。"),
            .itemdef = VEC_EMPTY,
            .data = VEC_EMPTY,
        };
        {
            DBItemDef def = {
                .type = 2000,
                .ITEMNAME = SV("ファイル名"),
                .VMEMO = SV(""),
                .LOADTYPE = 1,
                .CHOICE_NAME = VEC_EMPTY,
                .CHOICE_VAL = VEC_EMPTY,
                .DEFAULT_VAL = 0,
            };
            VEC_PUSH(def.CHOICE_NAME, SV("BGM"), arena);
            VEC_PUSH(def.CHOICE_VAL, 0, arena);
            VEC_PUSH(ty.itemdef, def, arena);
        }
        {
            DBItemDef def = {
                .type = 1000,
                .ITEMNAME = SV("再生音量[%] 標準100"),
                .VMEMO = SV(""),
                .LOADTYPE = 0,
                .CHOICE_NAME = VEC_EMPTY,
                .CHOICE_VAL = VEC_EMPTY,
                .DEFAULT_VAL = 100,
            };
            VEC_PUSH(ty.itemdef, def, arena);
        }
        {
            DBItemDef def = {
                .type = 1000,
                .ITEMNAME = SV("再生周波数[%]"),
                .VMEMO = SV(""),
                .LOADTYPE = 0,
                .CHOICE_NAME = VEC_EMPTY,
                .CHOICE_VAL = VEC_EMPTY,
                .DEFAULT_VAL = 100,
            };
            VEC_PUSH(ty.itemdef, def, arena);
        }
        {
            DBItemDef def = {
                .type = 1000,
                .ITEMNAME = SV("ループ開始位置(ms)"),
                .VMEMO = SV(""),
                .LOADTYPE = 0,
                .CHOICE_NAME = VEC_EMPTY,
                .CHOICE_VAL = VEC_EMPTY,
                .DEFAULT_VAL = 0,
            };
            VEC_PUSH(ty.itemdef, def, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        VEC_PUSH(db, ty, arena);
    }
    {
        DBType ty = {
            .TYPE_ID = 2,
            .TYPENAME = SV("BGSリスト"),
            .DATANAME_LOAD_TYPE = 0,
            .DATANAME_LOAD_NAME = SV(""),
            .MEMO = SV("[サウンド]コマンド内の<<\\n>>[ｼｽﾃﾑDBから直接選択]で<<\\n>>選べるBGSをここで指定します。<<\\n>>"),
            .itemdef = VEC_EMPTY,
            .data = VEC_EMPTY,
        };
        {
            DBItemDef def = {
                .type = 2000,
                .ITEMNAME = SV("ファイル名"),
                .VMEMO = SV(""),
                .LOADTYPE = 1,
                .CHOICE_NAME = VEC_EMPTY,
                .CHOICE_VAL = VEC_EMPTY,
                .DEFAULT_VAL = 0,
            };
            VEC_PUSH(def.CHOICE_NAME, SV("音楽"), arena);
            VEC_PUSH(def.CHOICE_VAL, 0, arena);
            VEC_PUSH(ty.itemdef, def, arena);
        }
        {
            DBItemDef def = {
                .type = 1000,
                .ITEMNAME = SV("再生音量[%]"),
                .VMEMO = SV(""),
                .LOADTYPE = 0,
                .CHOICE_NAME = VEC_EMPTY,
                .CHOICE_VAL = VEC_EMPTY,
                .DEFAULT_VAL = 100,
            };
            VEC_PUSH(ty.itemdef, def, arena);
        }
        {
            DBItemDef def = {
                .type = 1000,
                .ITEMNAME = SV("再生周波数[%]"),
                .VMEMO = SV(""),
                .LOADTYPE = 0,
                .CHOICE_NAME = VEC_EMPTY,
                .CHOICE_VAL = VEC_EMPTY,
                .DEFAULT_VAL = 100,
            };
            VEC_PUSH(ty.itemdef, def, arena);
        }
        {
            DBItemDef def = {
                .type = 1000,
                .ITEMNAME = SV("ループ開始位置(ms)"),
                .VMEMO = SV(""),
                .LOADTYPE = 0,
                .CHOICE_NAME = VEC_EMPTY,
                .CHOICE_VAL = VEC_EMPTY,
                .DEFAULT_VAL = 0,
            };
            VEC_PUSH(ty.itemdef, def, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        VEC_PUSH(db, ty, arena);
    }
    {
        DBType ty = {
            .TYPE_ID = 3,
            .TYPENAME = SV("SEリスト"),
            .DATANAME_LOAD_TYPE = 0,
            .DATANAME_LOAD_NAME = SV(""),
            .MEMO = SV("[サウンド]コマンド内の<<\\n>>[ｼｽﾃﾑDBから直接選択]で<<\\n>>選べるSE(効果音)を<<\\n>>ここで指定します。<<\\n>><<\\n>>[選択肢]コマンド実行時の<<\\n>>決定･ｶｰｿﾙ・ｷｬﾝｾﾙ音声として<<\\n>>データ0、1、2番が使われます。<<\\n>>(基本ｼｽﾃﾑでも使用されます)<<\\n>><<\\n>>「メモリ解放モード」は、<<\\n>>再生後メモリから<<\\n>>SEを解放するかどうか<<\\n>>決めるものです。<<\\n>>再生頻度が低いものは<<\\n>>1にしておくのがｵｽｽﾒです。<<\\n>>(ただし1で連続再生すると<<\\n>> そのたびに読込で<<\\n>> 引っかかりが発生します）"),
            .itemdef = VEC_EMPTY,
            .data = VEC_EMPTY,
        };
        {
            DBItemDef def = {
                .type = 2000,
                .ITEMNAME = SV("ファイル名"),
                .VMEMO = SV(""),
                .LOADTYPE = 1,
                .CHOICE_NAME = VEC_EMPTY,
                .CHOICE_VAL = VEC_EMPTY,
                .DEFAULT_VAL = 0,
            };
            VEC_PUSH(def.CHOICE_NAME, SV("効果音"), arena);
            VEC_PUSH(def.CHOICE_VAL, 0, arena);
            VEC_PUSH(ty.itemdef, def, arena);
        }
        {
            DBItemDef def = {
                .type = 1000,
                .ITEMNAME = SV("再生音量[%]"),
                .VMEMO = SV(""),
                .LOADTYPE = 0,
                .CHOICE_NAME = VEC_EMPTY,
                .CHOICE_VAL = VEC_EMPTY,
                .DEFAULT_VAL = 100,
            };
            VEC_PUSH(ty.itemdef, def, arena);
        }
        {
            DBItemDef def = {
                .type = 1000,
                .ITEMNAME = SV("再生周波数[%]"),
                .VMEMO = SV(""),
                .LOADTYPE = 0,
                .CHOICE_NAME = VEC_EMPTY,
                .CHOICE_VAL = VEC_EMPTY,
                .DEFAULT_VAL = 100,
            };
            VEC_PUSH(ty.itemdef, def, arena);
        }
        {
            DBItemDef def = {
                .type = 1000,
                .ITEMNAME = SV("ﾒﾓﾘ解放モード"),
                .VMEMO = SV(""),
                .LOADTYPE = 3,
                .CHOICE_NAME = VEC_EMPTY,
                .CHOICE_VAL = VEC_EMPTY,
                .DEFAULT_VAL = 1,
            };
            VEC_PUSH(def.CHOICE_NAME, SV("解放しない"), arena);
            VEC_PUSH(def.CHOICE_NAME, SV("再生後、解放する"), arena);
            VEC_PUSH(def.CHOICE_VAL, 0, arena);
            VEC_PUSH(def.CHOICE_VAL, 1, arena);
            VEC_PUSH(ty.itemdef, def, arena);
        }
        {
            DBData data = {
                .name = SV("[ｼｽﾃﾑ/選択肢]決定音"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("[ｼｽﾃﾑ/選択肢]カーソル移動"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("[ｼｽﾃﾑ/選択肢]キャンセル音"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 1}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 1}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 1}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 1}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 1}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 1}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 1}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 1}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 1}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 1}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 1}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 1}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 1}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 1}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 1}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 1}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 1}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        VEC_PUSH(db, ty, arena);
    }
    {
        DBType ty = {
            .TYPE_ID = 4,
            .TYPENAME = SV("文字変数名"),
            .DATANAME_LOAD_TYPE = 0,
            .DATANAME_LOAD_NAME = SV(""),
            .MEMO = SV("文字列変数名の一覧です。<<\\n>><<\\n>>[文字列操作]コマンド欄の<<\\n>>下部からでも書き込み可能です。"),
            .itemdef = VEC_EMPTY,
            .data = VEC_EMPTY,
        };
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        VEC_PUSH(db, ty, arena);
    }
    {
        DBType ty = {
            .TYPE_ID = 5,
            .TYPENAME = SV("システム文字列"),
            .DATANAME_LOAD_TYPE = 0,
            .DATANAME_LOAD_NAME = SV(""),
            .MEMO = SV("システム文字列名です。<<\\n>>このデータは変更できません。<<\\n>><<\\n>>([文字列操作]コマンド時などに<<\\n>> SysS??としてこれらのデータの<<\\n>> 名前が表示されています)"),
            .itemdef = VEC_EMPTY,
            .data = VEC_EMPTY,
        };
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("決定ｷｰ名[ｷｰﾎﾞｰﾄﾞ]"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("ｷｬﾝｾﾙｷｰ名[ｷｰﾎﾞｰﾄﾞ]"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("ｻﾌﾞｷｰ名[ｷｰﾎﾞｰﾄﾞ]"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("決定ｷｰ名[パッド]"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("ｷｬﾝｾﾙｷｰ名[パッド]"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("ｻﾌﾞｷｰ名[パッド]"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("主人公 キャラ画像名"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("仲間1 キャラ画像名"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("仲間2 キャラ画像名"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("仲間3 キャラ画像名"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("仲間4 キャラ画像名"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("仲間5 キャラ画像名"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("[読]ｻｰﾄﾞﾊﾟｰﾃｨ実行識別ID"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("Proxy アドレス[例:aa.net]"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("Proxy ポート番号[例:80]"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("[読]ダウンロード中データ内容"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("[読]文章表示の内容"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("文章表示追加 先頭"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("文章表示追加 最後尾"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("遠景の画像ﾌｧｲﾙ名"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("フォグの画像ﾌｧｲﾙ名"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("[読]再生中BGMﾌｧｲﾙ名"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("[読]再生中BGSﾌｧｲﾙ名(ﾁｬﾝﾈﾙ=Sys99)"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("[読]選択肢1の内容"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("[読]選択肢2の内容"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("[読]選択肢3の内容"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("[読]選択肢4の内容"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("[読]選択肢5の内容"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("[読]選択肢6の内容"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("[読]選択肢7の内容"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("[読]選択肢8の内容"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("[読]選択肢9の内容"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("[読]選択肢10の内容"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("[読]起動データフォルダ"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("[読]最終緑帯メッセージ文"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("[読]Ev実行経路"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("[P]クリップボード内容"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("キーボード入力中文字列"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("[読]ゲームパッドデバイス名"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("[読]ゲームパッドの種別"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("[読]Basicﾃﾞｰﾀﾌｫﾙﾀﾞ"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("[読]Game.exe起動時引数"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("[読]OSの言語名"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("[読]ゲーム名 メイン"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("[読]ゲーム名 追記"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("[読]ﾊﾟﾌｫｰﾏﾝｽﾓﾆﾀｰ最終出力文"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        VEC_PUSH(db, ty, arena);
    }
    {
        DBType ty = {
            .TYPE_ID = 6,
            .TYPENAME = SV("システム変数名"),
            .DATANAME_LOAD_TYPE = 0,
            .DATANAME_LOAD_NAME = SV(""),
            .MEMO = SV("システム変数名です。<<\\n>>このデータは変更できません。<<\\n>><<\\n>>([変数操作]コマンド時などに<<\\n>> [Sys]変数としてこれらのデータの<<\\n>> 名前が表示されています)"),
            .itemdef = VEC_EMPTY,
            .data = VEC_EMPTY,
        };
        {
            DBData data = {
                .name = SV("顔ｸﾞﾗﾌｨｯｸ番号"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("ﾒｯｾｰｼﾞｳｨﾝﾄﾞｳ X座標"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("ﾒｯｾｰｼﾞｳｨﾝﾄﾞｳ Y座標"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("選択肢ｳｨﾝﾄﾞｳ X座標"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("選択肢ｳｨﾝﾄﾞｳ Y座標"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("ﾎﾟｰｽﾞｶｰｿﾙX (Sys1に足した位置)"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("ﾎﾟｰｽﾞｶｰｿﾙY (Sys2に足した位置)"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("選択肢次回初期位置(0<<COMMA>>1<<COMMA>>2...)"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("基本フォントサイズ"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("ﾒｯｾｰｼﾞ速度( X 文字/秒)"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("ﾒｯｾｰｼﾞ表示ｳｪｲﾄ(X ﾌﾚｰﾑ)"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("ﾋﾟｸﾁｬ文字速度( X 文字/秒)"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("文章の表示 実行中？(1=ON)"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("ｲﾍﾞﾝﾄ実行中？(1=ON)"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("選択肢ｳｨﾝﾄﾞｳ表示する?(1=ON)"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("方向ｷｰﾘﾋﾟｰﾄ初ｳｪｲﾄ(ﾌﾚｰﾑ数指定)"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("方向ｷｰﾘﾋﾟｰﾄ次ｳｪｲﾄ(ﾌﾚｰﾑ数指定)"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("選択肢SE番号：決定(Sys3)"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("選択肢SE番号：選択(Sys3)"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("選択肢SE番号：ｷｬﾝｾﾙ(Sys3)"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("選択肢ｶｰｿﾙ現位置(0<<COMMA>>1<<COMMA>>2..非表示時:-1)"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("文字影付ける?(0=OFF<<COMMA>>1以上=ON)"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("選択肢ｶｰｿﾙ表示ﾓｰﾄﾞ?(0=通常<<COMMA>>1=加算)"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("ルビのフォントサイズ"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("[読]ｾｰﾌﾞﾃﾞｰﾀ読込判定(1=成功 0=失敗)"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("現在ｾｰﾌﾞﾃﾞｰﾀ番号(0～)"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("選択肢ｳｨﾝﾄﾞｳ X余白"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("選択肢ｳｨﾝﾄﾞｳ Y余白"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("経過ﾌﾚｰﾑ数[1000万で一周]"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("プレイ時間（1秒単位）"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("[読]プレイ時間(ﾐﾘ秒単位)"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("[読]内部FPS(毎秒変化）"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("[読]描画FPS(毎秒変化）"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("ﾌﾚｰﾑｽｷｯﾌﾟﾚﾍﾞﾙ(0～2)"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("ｷｬﾗ表示Y補正[-(ﾀｲﾙ-1)～0]px"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("主人公移動中？(1=YES) ﾏｽの間にいる?"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("主人公の影番号(Sys9)"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("仲間の車間距離(1あたり1歩)"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("ｷｰﾎﾞ入力中受付可?[1:ﾏｳｽﾀｯﾁ+2ﾊﾟｯﾄﾞ] *[ｷｰ入力]のみ"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("[読]ｷｰﾎﾞ入力状態 0=Enter -1=ｷｬﾝｾﾙ"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("ｷｰﾎﾞ入力文字列 X座標"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("ｷｰﾎﾞ入力文字列 Y座標"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("ｷｰﾎﾞ入力文字ｱﾝﾁｴｲﾘｱｽ[0無/1有/+2ｴｯｼﾞ]"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("[読]ﾈｯﾄ/DL済ｻｲｽﾞ(byte)"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("[読]ﾈｯﾄ/DL予定ｻｲｽﾞ(byte)[不明なら-1]"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("[読]ﾈｯﾄ/接続速度"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("[読]ﾈｯﾄ/接続時間(秒)"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("[読]ﾈｯﾄ/残り時間(秒)"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("[読]ﾈｯﾄ/状態 -1失敗 0通信中 1終了"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("◆ 以下 ｷｰｺﾝﾌｨｸﾞ ◆"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("決定ｷｰ(ｷｰﾎﾞｰﾄﾞ)"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("ｷｬﾝｾﾙｷｰ(ｷｰﾎﾞｰﾄﾞ)"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("ｻﾌﾞｷｰ（ｷｰﾎﾞｰﾄﾞ）"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("決定ｷｰ（ﾊﾟｯﾄﾞ）"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("ｷｬﾝｾﾙｷｰ（ﾊﾟｯﾄﾞ）"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("ｻﾌﾞｷｰ（ﾊﾟｯﾄﾞ）"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("◆ここまでｷｰｺﾝﾌｨｸﾞ◆"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("決定ｷｰﾘﾋﾟｰﾄする？(1=ON) *[キー待ち]用"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("ｷｬﾝｾﾙｷｰﾘﾋﾟｰﾄする？(1=ON) *[キー待ち]用"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("フォグ番号(Sys13)"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("フォグX速度"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("フォグY速度"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("ﾌｫｸﾞ描画ﾀｲﾌﾟ(0通常1加算2減算)"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("ﾌｫｸﾞ不透明度(0～255)"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("遠景番号(Sys13)"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("遠景X速度"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("遠景Y速度"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("マウスＸ位置"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("マウスＹ位置"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("文･選択肢のﾏｳｽ入力(1なら受付)"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("ﾏｳｽﾎﾟｲﾝﾀ表示する?(1=ON)"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("現在の[年]"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("現在の[月]"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("現在の[日]"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("現在の[時]"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("現在の[分]"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("現在の[秒]"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("画面ｻｲｽﾞ(0=320x240<<COMMA>>1=640x480<<COMMA>>2=800x600 他-1)"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("描画ﾓｰﾄﾞ(0=3Dﾓｰﾄﾞ<<COMMA>>1=ｿﾌﾄｳｪｱ)"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("仲間1影番号(-1=主人公と同じ）"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("仲間2影番号(-1=主人公と同じ）"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("仲間3影番号(-1=主人公と同じ）"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("仲間4影番号(-1=主人公と同じ）"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("仲間5影番号(-1=主人公と同じ）"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("再生中BGMパン[-255～255]"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("再生中BGSパン(Ch=Sys99)[-255～255]"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("再生中BGM音量[%]"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("再生中BGMテンポ/周波数[%]"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("再生中BGS音量(Ch=Sys99)[%]"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("再生中BGS周波数(Ch=Sys99)[%]"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("対象とするBGSﾁｬﾝﾈﾙ[Ch]"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("BGM音量補正[%]"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("BGS音量補正[%]"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("SE音量補正[%]"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("Ｘスクロール値"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("Ｙスクロール値"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("[読]現ﾌﾚｰﾑ開始からのｺﾏﾝﾄﾞ処理数"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("現在の乱数のシード(種)"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("[読]画面アクティブ状態(1=アクティブ)"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("[読]テストプレイ中？(1=YES)"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("[読]動作ﾊﾞｰｼﾞｮﾝ調整(x100)"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("[読]Game.exeバージョン(x100)"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("[読]画面サイズＸ"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("[読]画面サイズＹ"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("[読]タイルサイズ"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("[読]基本画面倍率x[1-3]"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("ﾘｾｯﾄ履歴 0=ﾅｼ/1=F12/2=[ﾀｲﾄﾙ画面へ]ｺﾏﾝﾄﾞ"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("Map&Evｽﾞｰﾑ時なめらか=1[α版]"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("次表示ﾋﾟｸﾁｬ拡縮くっきり(1=YES)"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("フォント太さ(1～10)"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("Mapﾀｲﾙ非ｾｰﾌﾞﾌﾗｸﾞ(1=YES)"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("Ev斜め移動 X速度補正[%]"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("Ev斜め移動 Y速度補正[%]"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("主人公 中心X補正"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("主人公 中心Y補正"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("ﾊﾟｯﾄﾞPOVｷｰ方向入力ｵﾝ？[1=ON]"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("ﾊﾟｯﾄﾞ左ｽﾃｨｯｸで方向入力ｵﾝ？[1=ON]"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("精密ｳｨﾝﾄﾞｳ拡大率[0.1%](-1=自動)"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("ｳｨﾝﾄﾞｳ拡大率[%](-1=自動)"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("画面ﾓｰﾄﾞ[ｳｨﾝﾄﾞｳ:0/仮想全:1/全:2]"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("ｳｨﾝﾄﾞｳ拡縮ﾓｰﾄﾞ[くっきり=0/なめらか=1]"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("[読]処理可能ｺﾏﾝﾄﾞ[通常0/プロ版1]"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("DirectInputならPS/Swiｺﾝ準拠?=1(1推奨)"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("特殊文字変換を無効化[1=ON]"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("[読]ｷｬﾗｸﾀｰ移動可能方向[4/8] ※ｹﾞｰﾑ基本設定"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("[読]ｷｬﾗｸﾀｰ移動幅[0=0.5/1=1ﾏｽ] ※ｹﾞｰﾑ基本設定"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("[読]横方向の字詰めピクセル[px] ※ｹﾞｰﾑ基本設定"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("[読]改行の間隔[px] ※ｹﾞｰﾑ基本設定"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("[読]選択肢の改行間隔 ※ｹﾞｰﾑ基本設定"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("[読]最大マップレイヤー数"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("ﾀｯﾁ操作ﾏｳｽ変換[1=ON/ﾀｯﾁﾃﾞﾊﾞｲｽ用]"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" -----"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        VEC_PUSH(db, ty, arena);
    }
    {
        DBType ty = {
            .TYPE_ID = 7,
            .TYPENAME = SV("位置設定リスト"),
            .DATANAME_LOAD_TYPE = 0,
            .DATANAME_LOAD_NAME = SV(""),
            .MEMO = SV("[場所移動]コマンド入力時に<<\\n>>選べる場所移動先をここで<<\\n>>指定できます。<<\\n>><<\\n>>「ゲーム開始時の初期位置」<<\\n>>として[データ0番]の位置が<<\\n>>使用されます【重要!!】<<\\n>><<\\n>>マップ編集時の<<\\n>>[ゲーム開始位置に設定]<<\\n>>[この位置をDBに保存]<<\\n>>でもここに書き込まれます。"),
            .itemdef = VEC_EMPTY,
            .data = VEC_EMPTY,
        };
        {
            DBItemDef def = {
                .type = 1000,
                .ITEMNAME = SV("マップID"),
                .VMEMO = SV(""),
                .LOADTYPE = 2,
                .CHOICE_NAME = VEC_EMPTY,
                .CHOICE_VAL = VEC_EMPTY,
                .DEFAULT_VAL = 0,
            };
            VEC_PUSH(def.CHOICE_VAL, 0, arena);
            VEC_PUSH(def.CHOICE_VAL, 0, arena);
            VEC_PUSH(def.CHOICE_VAL, 0, arena);
            VEC_PUSH(ty.itemdef, def, arena);
        }
        {
            DBItemDef def = {
                .type = 1000,
                .ITEMNAME = SV("X座標(マップ位置)"),
                .VMEMO = SV(""),
                .LOADTYPE = 0,
                .CHOICE_NAME = VEC_EMPTY,
                .CHOICE_VAL = VEC_EMPTY,
                .DEFAULT_VAL = 0,
            };
            VEC_PUSH(ty.itemdef, def, arena);
        }
        {
            DBItemDef def = {
                .type = 1000,
                .ITEMNAME = SV("Y座標(マップ位置)"),
                .VMEMO = SV(""),
                .LOADTYPE = 0,
                .CHOICE_NAME = VEC_EMPTY,
                .CHOICE_VAL = VEC_EMPTY,
                .DEFAULT_VAL = 0,
            };
            VEC_PUSH(ty.itemdef, def, arena);
        }
        {
            DBData data = {
                .name = SV("初期位置"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        VEC_PUSH(db, ty, arena);
    }
    {
        DBType ty = {
            .TYPE_ID = 8,
            .TYPENAME = SV("キャラクター画像"),
            .DATANAME_LOAD_TYPE = 0,
            .DATANAME_LOAD_NAME = SV(""),
            .MEMO = SV("[ｷｬﾗ動作指定]内で<<\\n>>[グラフィック変更]コマンドの<<\\n>>キャラ画像として選べる<<\\n>>画像リストです。<<\\n>><<\\n>>ただし現在では、<<\\n>>[エフェクト]コマンド内の<<\\n>>[キャラ]のエフェクト→<<\\n>>[キャラチップ変更]処理を<<\\n>>使って画像変更するほうが<<\\n>>楽だと思います。"),
            .itemdef = VEC_EMPTY,
            .data = VEC_EMPTY,
        };
        {
            DBItemDef def = {
                .type = 2000,
                .ITEMNAME = SV("読込ファイル名"),
                .VMEMO = SV(""),
                .LOADTYPE = 1,
                .CHOICE_NAME = VEC_EMPTY,
                .CHOICE_VAL = VEC_EMPTY,
                .DEFAULT_VAL = 0,
            };
            VEC_PUSH(def.CHOICE_NAME, SV("キャラチップ"), arena);
            VEC_PUSH(def.CHOICE_VAL, 0, arena);
            VEC_PUSH(ty.itemdef, def, arena);
        }
        {
            DBData data = {
                .name = SV("※空データ"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("CharaChip\tesT.png")}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        VEC_PUSH(db, ty, arena);
    }
    {
        DBType ty = {
            .TYPE_ID = 9,
            .TYPENAME = SV("キャラクター影画像"),
            .DATANAME_LOAD_TYPE = 0,
            .DATANAME_LOAD_NAME = SV(""),
            .MEMO = SV("[マップイベントウィンドウ]内で<<\\n>>設定できる「影グラフィック」<<\\n>>をここで指定します。<<\\n>><<\\n>>画像はゲーム内で<<\\n>>[キャラクターの影]として<<\\n>>表示されます。<<\\n>><<\\n>>影番号は「255」番まで<<\\n>>使用可能です。<<\\n>>(それ以上設定しても<<\\n>>　使用できません)"),
            .itemdef = VEC_EMPTY,
            .data = VEC_EMPTY,
        };
        {
            DBItemDef def = {
                .type = 2000,
                .ITEMNAME = SV("読込ファイル名"),
                .VMEMO = SV(""),
                .LOADTYPE = 1,
                .CHOICE_NAME = VEC_EMPTY,
                .CHOICE_VAL = VEC_EMPTY,
                .DEFAULT_VAL = 0,
            };
            VEC_PUSH(def.CHOICE_NAME, SV("BasicGraphic"), arena);
            VEC_PUSH(def.CHOICE_VAL, 0, arena);
            VEC_PUSH(ty.itemdef, def, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        VEC_PUSH(db, ty, arena);
    }
    {
        DBType ty = {
            .TYPE_ID = 10,
            .TYPENAME = SV("ウィンドウ画像"),
            .DATANAME_LOAD_TYPE = 0,
            .DATANAME_LOAD_NAME = SV(""),
            .MEMO = SV("[文章の表示]コマンド実行時の<<\\n>>[入力待ちポーズ画像]として<<\\n>>データ0番が使用されます。<<\\n>><<\\n>>[選択肢]コマンド実行時の<<\\n>>[選択肢ウィンドウ画像]と<<\\n>>[選択肢カーソル画像]として<<\\n>>データ1番と2番が使用されます。<<\\n>><<\\n>>※[データ1:選択肢ｳｨﾝﾄﾞｳ]は<<\\n>>　3x3に分割され、画像が<<\\n>>　引き延ばされて使用されます。<<\\n>>([ﾋﾟｸﾁｬ]の[お手軽ｳｨﾝﾄﾞｳ]<<\\n>>　と同じ)"),
            .itemdef = VEC_EMPTY,
            .data = VEC_EMPTY,
        };
        {
            DBItemDef def = {
                .type = 2000,
                .ITEMNAME = SV("画像ファイル名"),
                .VMEMO = SV(""),
                .LOADTYPE = 1,
                .CHOICE_NAME = VEC_EMPTY,
                .CHOICE_VAL = VEC_EMPTY,
                .DEFAULT_VAL = 0,
            };
            VEC_PUSH(def.CHOICE_NAME, SV("BasicGraphic"), arena);
            VEC_PUSH(def.CHOICE_VAL, 0, arena);
            VEC_PUSH(ty.itemdef, def, arena);
        }
        {
            DBItemDef def = {
                .type = 1000,
                .ITEMNAME = SV("縦分割数(一部のみ)"),
                .VMEMO = SV(""),
                .LOADTYPE = 0,
                .CHOICE_NAME = VEC_EMPTY,
                .CHOICE_VAL = VEC_EMPTY,
                .DEFAULT_VAL = 0,
            };
            VEC_PUSH(ty.itemdef, def, arena);
        }
        {
            DBData data = {
                .name = SV("入力待ちﾎﾟｰｽﾞ画像[縦分割]"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 1}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("選択肢ｳｨﾝﾄﾞｳ画像[縦分割無効]"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("選択肢カーソル指定[縦分割]"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 1}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        VEC_PUSH(db, ty, arena);
    }
    {
        DBType ty = {
            .TYPE_ID = 11,
            .TYPENAME = SV("ﾄﾗﾝｼﾞｼｮﾝﾀｲﾌﾟ"),
            .DATANAME_LOAD_TYPE = 0,
            .DATANAME_LOAD_NAME = SV(""),
            .MEMO = SV("[イベント制御]の<<\\n>>[トランジションの指定]コマンドで<<\\n>>選択できるトランジション画像を<<\\n>>ここで指定します。<<\\n>><<\\n>>トランジションとは<<\\n>>画面の切り替わり演出です。<<\\n>>画像はグレースケールで<<\\n>>作成する必要があります。"),
            .itemdef = VEC_EMPTY,
            .data = VEC_EMPTY,
        };
        {
            DBItemDef def = {
                .type = 2000,
                .ITEMNAME = SV("ﾌｧｲﾙ名(ﾓﾉｸﾛ)"),
                .VMEMO = SV(""),
                .LOADTYPE = 1,
                .CHOICE_NAME = VEC_EMPTY,
                .CHOICE_VAL = VEC_EMPTY,
                .DEFAULT_VAL = 0,
            };
            VEC_PUSH(def.CHOICE_NAME, SV("BasicGraphic"), arena);
            VEC_PUSH(def.CHOICE_VAL, 0, arena);
            VEC_PUSH(ty.itemdef, def, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        VEC_PUSH(db, ty, arena);
    }
    {
        DBType ty = {
            .TYPE_ID = 12,
            .TYPENAME = SV("文字色"),
            .DATANAME_LOAD_TYPE = 0,
            .DATANAME_LOAD_NAME = SV(""),
            .MEMO = SV("文章中に\\c[?]等で指定できる<<\\n>>文字色をここで指定します。<<\\n>><<\\n>>[文字列操作]の<<\\n>>[キーボード入力]実行時に、<<\\n>> 10番:ｷｰﾎﾞｰﾄﾞ入力文字色<<\\n>> 11番:ｷｰﾎﾞｰﾄﾞ変換背景色<<\\n>>が使用されます。<<\\n>><<\\n>>文章中で\\r[A,B]（ルビ）を<<\\n>>使用時、ルビの部分に<<\\n>> 13番:ルビ用文字色<<\\n>>が使用されます。"),
            .itemdef = VEC_EMPTY,
            .data = VEC_EMPTY,
        };
        {
            DBItemDef def = {
                .type = 1000,
                .ITEMNAME = SV("赤(0-255)"),
                .VMEMO = SV(""),
                .LOADTYPE = 0,
                .CHOICE_NAME = VEC_EMPTY,
                .CHOICE_VAL = VEC_EMPTY,
                .DEFAULT_VAL = 0,
            };
            VEC_PUSH(ty.itemdef, def, arena);
        }
        {
            DBItemDef def = {
                .type = 1000,
                .ITEMNAME = SV("緑(0-255)"),
                .VMEMO = SV(""),
                .LOADTYPE = 0,
                .CHOICE_NAME = VEC_EMPTY,
                .CHOICE_VAL = VEC_EMPTY,
                .DEFAULT_VAL = 0,
            };
            VEC_PUSH(ty.itemdef, def, arena);
        }
        {
            DBItemDef def = {
                .type = 1000,
                .ITEMNAME = SV("青(0-255)"),
                .VMEMO = SV(""),
                .LOADTYPE = 0,
                .CHOICE_NAME = VEC_EMPTY,
                .CHOICE_VAL = VEC_EMPTY,
                .DEFAULT_VAL = 0,
            };
            VEC_PUSH(ty.itemdef, def, arena);
        }
        {
            DBData data = {
                .name = SV("デフォルト色"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.int_val = 255}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 255}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 255}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("[Sys/文操作]ｷｰﾎﾞ入力文字色"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.int_val = 255}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 255}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 255}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("[Sys/文操作]ｷｰﾎﾞ変換中背景色"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.int_val = 170}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 170}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 200}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(" (×現在無効?) [Sys]ｷｰﾎﾞ入力下線"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.int_val = 255}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 100}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV("[Sys/特殊文字]ルビ用文字色"),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.int_val = 255}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 255}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 255}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        VEC_PUSH(db, ty, arena);
    }
    {
        DBType ty = {
            .TYPE_ID = 13,
            .TYPENAME = SV("遠景･フォグ画像"),
            .DATANAME_LOAD_TYPE = 0,
            .DATANAME_LOAD_NAME = SV(""),
            .MEMO = SV("[マップ新規作成][マップ設定]時<<\\n>>などで、[遠景]または<<\\n>>[フォグ]画像として選べる画像や<<\\n>>設定をここで指定します。"),
            .itemdef = VEC_EMPTY,
            .data = VEC_EMPTY,
        };
        {
            DBItemDef def = {
                .type = 2000,
                .ITEMNAME = SV("画像ファイル名"),
                .VMEMO = SV(""),
                .LOADTYPE = 1,
                .CHOICE_NAME = VEC_EMPTY,
                .CHOICE_VAL = VEC_EMPTY,
                .DEFAULT_VAL = 0,
            };
            VEC_PUSH(def.CHOICE_NAME, SV("Fog_BackGround"), arena);
            VEC_PUSH(def.CHOICE_VAL, 0, arena);
            VEC_PUSH(ty.itemdef, def, arena);
        }
        {
            DBItemDef def = {
                .type = 1000,
                .ITEMNAME = SV("X移動速度"),
                .VMEMO = SV(""),
                .LOADTYPE = 0,
                .CHOICE_NAME = VEC_EMPTY,
                .CHOICE_VAL = VEC_EMPTY,
                .DEFAULT_VAL = 0,
            };
            VEC_PUSH(ty.itemdef, def, arena);
        }
        {
            DBItemDef def = {
                .type = 1000,
                .ITEMNAME = SV("Y移動速度"),
                .VMEMO = SV(""),
                .LOADTYPE = 0,
                .CHOICE_NAME = VEC_EMPTY,
                .CHOICE_VAL = VEC_EMPTY,
                .DEFAULT_VAL = 0,
            };
            VEC_PUSH(ty.itemdef, def, arena);
        }
        {
            DBItemDef def = {
                .type = 1000,
                .ITEMNAME = SV("マップと位置連動する？"),
                .VMEMO = SV(""),
                .LOADTYPE = 3,
                .CHOICE_NAME = VEC_EMPTY,
                .CHOICE_VAL = VEC_EMPTY,
                .DEFAULT_VAL = 1,
            };
            VEC_PUSH(def.CHOICE_NAME, SV("位置連動あり"), arena);
            VEC_PUSH(def.CHOICE_NAME, SV("位置連動なし"), arena);
            VEC_PUSH(def.CHOICE_VAL, 1, arena);
            VEC_PUSH(def.CHOICE_VAL, 0, arena);
            VEC_PUSH(ty.itemdef, def, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 1}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 1}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(data.values, (DBVal){.int_val = 0}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        VEC_PUSH(db, ty, arena);
    }
    {
        DBType ty = {
            .TYPE_ID = 14,
            .TYPENAME = SV("通常変数名"),
            .DATANAME_LOAD_TYPE = 0,
            .DATANAME_LOAD_NAME = SV(""),
            .MEMO = SV("変数名の一覧です。<<\\n>><<\\n>>[変数操作]コマンド欄の<<\\n>>下部からでも書き込み可能です。"),
            .itemdef = VEC_EMPTY,
            .data = VEC_EMPTY,
        };
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        VEC_PUSH(db, ty, arena);
    }
    {
        DBType ty = {
            .TYPE_ID = 15,
            .TYPENAME = SV("予備変数1"),
            .DATANAME_LOAD_TYPE = 0,
            .DATANAME_LOAD_NAME = SV(""),
            .MEMO = SV("変数名の一覧です。<<\\n>><<\\n>>[変数操作]コマンド欄の<<\\n>>下部からでも書き込み可能です。"),
            .itemdef = VEC_EMPTY,
            .data = VEC_EMPTY,
        };
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        VEC_PUSH(db, ty, arena);
    }
    {
        DBType ty = {
            .TYPE_ID = 16,
            .TYPENAME = SV("予備変数2"),
            .DATANAME_LOAD_TYPE = 0,
            .DATANAME_LOAD_NAME = SV(""),
            .MEMO = SV("変数名の一覧です。<<\\n>><<\\n>>[変数操作]コマンド欄の<<\\n>>下部からでも書き込み可能です。"),
            .itemdef = VEC_EMPTY,
            .data = VEC_EMPTY,
        };
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        VEC_PUSH(db, ty, arena);
    }
    {
        DBType ty = {
            .TYPE_ID = 17,
            .TYPENAME = SV("予備変数3"),
            .DATANAME_LOAD_TYPE = 0,
            .DATANAME_LOAD_NAME = SV(""),
            .MEMO = SV("変数名の一覧です。<<\\n>><<\\n>>[変数操作]コマンド欄の<<\\n>>下部からでも書き込み可能です。"),
            .itemdef = VEC_EMPTY,
            .data = VEC_EMPTY,
        };
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        VEC_PUSH(db, ty, arena);
    }
    {
        DBType ty = {
            .TYPE_ID = 18,
            .TYPENAME = SV("予備変数4"),
            .DATANAME_LOAD_TYPE = 0,
            .DATANAME_LOAD_NAME = SV(""),
            .MEMO = SV("変数名の一覧です。<<\\n>><<\\n>>[変数操作]コマンド欄の<<\\n>>下部からでも書き込み可能です。"),
            .itemdef = VEC_EMPTY,
            .data = VEC_EMPTY,
        };
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        VEC_PUSH(db, ty, arena);
    }
    {
        DBType ty = {
            .TYPE_ID = 19,
            .TYPENAME = SV("予備変数5"),
            .DATANAME_LOAD_TYPE = 0,
            .DATANAME_LOAD_NAME = SV(""),
            .MEMO = SV("変数名の一覧です。<<\\n>><<\\n>>[変数操作]コマンド欄の<<\\n>>下部からでも書き込み可能です。"),
            .itemdef = VEC_EMPTY,
            .data = VEC_EMPTY,
        };
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        VEC_PUSH(db, ty, arena);
    }
    {
        DBType ty = {
            .TYPE_ID = 20,
            .TYPENAME = SV("予備変数6"),
            .DATANAME_LOAD_TYPE = 0,
            .DATANAME_LOAD_NAME = SV(""),
            .MEMO = SV("変数名の一覧です。<<\\n>><<\\n>>[変数操作]コマンド欄の<<\\n>>下部からでも書き込み可能です。"),
            .itemdef = VEC_EMPTY,
            .data = VEC_EMPTY,
        };
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        VEC_PUSH(db, ty, arena);
    }
    {
        DBType ty = {
            .TYPE_ID = 21,
            .TYPENAME = SV("予備変数7"),
            .DATANAME_LOAD_TYPE = 0,
            .DATANAME_LOAD_NAME = SV(""),
            .MEMO = SV("変数名の一覧です。<<\\n>><<\\n>>[変数操作]コマンド欄の<<\\n>>下部からでも書き込み可能です。"),
            .itemdef = VEC_EMPTY,
            .data = VEC_EMPTY,
        };
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        VEC_PUSH(db, ty, arena);
    }
    {
        DBType ty = {
            .TYPE_ID = 22,
            .TYPENAME = SV("予備変数8"),
            .DATANAME_LOAD_TYPE = 0,
            .DATANAME_LOAD_NAME = SV(""),
            .MEMO = SV("変数名の一覧です。<<\\n>><<\\n>>[変数操作]コマンド欄の<<\\n>>下部からでも書き込み可能です。"),
            .itemdef = VEC_EMPTY,
            .data = VEC_EMPTY,
        };
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        VEC_PUSH(db, ty, arena);
    }
    {
        DBType ty = {
            .TYPE_ID = 23,
            .TYPENAME = SV("予備変数9"),
            .DATANAME_LOAD_TYPE = 0,
            .DATANAME_LOAD_NAME = SV(""),
            .MEMO = SV("変数名の一覧です。<<\\n>><<\\n>>[変数操作]コマンド欄の<<\\n>>下部からでも書き込み可能です。"),
            .itemdef = VEC_EMPTY,
            .data = VEC_EMPTY,
        };
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(ty.data, data, arena);
        }
        VEC_PUSH(db, ty, arena);
    }
    {
        DBType ty = {
            .TYPE_ID = 24,
            .TYPENAME = SV("顔グラフィック名"),
            .DATANAME_LOAD_TYPE = 0,
            .DATANAME_LOAD_NAME = SV(""),
            .MEMO = SV("顔グラフィック名の一覧です。<<\\n>><<\\n>>[文章の表示]コマンド欄下部に<<\\n>>番号を入れると、ここに記入した<<\\n>>[ID]名を確認することができます。<<\\n>>"),
            .itemdef = VEC_EMPTY,
            .data = VEC_EMPTY,
        };
        {
            DBItemDef def = {
                .type = 2000,
                .ITEMNAME = SV("顔画像ファイル"),
                .VMEMO = SV(""),
                .LOADTYPE = 1,
                .CHOICE_NAME = VEC_EMPTY,
                .CHOICE_VAL = VEC_EMPTY,
                .DEFAULT_VAL = 0,
            };
            VEC_PUSH(def.CHOICE_NAME, SV("ピクチャ"), arena);
            VEC_PUSH(def.CHOICE_VAL, 0, arena);
            VEC_PUSH(ty.itemdef, def, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        VEC_PUSH(db, ty, arena);
    }
    {
        DBType ty = {
            .TYPE_ID = 25,
            .TYPENAME = SV(""),
            .DATANAME_LOAD_TYPE = 0,
            .DATANAME_LOAD_NAME = SV(""),
            .MEMO = SV(""),
            .itemdef = VEC_EMPTY,
            .data = VEC_EMPTY,
        };
        {
            DBItemDef def = {
                .type = 2000,
                .ITEMNAME = SV(""),
                .VMEMO = SV(""),
                .LOADTYPE = 0,
                .CHOICE_NAME = VEC_EMPTY,
                .CHOICE_VAL = VEC_EMPTY,
                .DEFAULT_VAL = 0,
            };
            VEC_PUSH(ty.itemdef, def, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        VEC_PUSH(db, ty, arena);
    }
    {
        DBType ty = {
            .TYPE_ID = 26,
            .TYPENAME = SV(""),
            .DATANAME_LOAD_TYPE = 0,
            .DATANAME_LOAD_NAME = SV(""),
            .MEMO = SV(""),
            .itemdef = VEC_EMPTY,
            .data = VEC_EMPTY,
        };
        {
            DBItemDef def = {
                .type = 2000,
                .ITEMNAME = SV(""),
                .VMEMO = SV(""),
                .LOADTYPE = 0,
                .CHOICE_NAME = VEC_EMPTY,
                .CHOICE_VAL = VEC_EMPTY,
                .DEFAULT_VAL = 0,
            };
            VEC_PUSH(ty.itemdef, def, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        VEC_PUSH(db, ty, arena);
    }
    {
        DBType ty = {
            .TYPE_ID = 27,
            .TYPENAME = SV(""),
            .DATANAME_LOAD_TYPE = 0,
            .DATANAME_LOAD_NAME = SV(""),
            .MEMO = SV(""),
            .itemdef = VEC_EMPTY,
            .data = VEC_EMPTY,
        };
        {
            DBItemDef def = {
                .type = 2000,
                .ITEMNAME = SV(""),
                .VMEMO = SV(""),
                .LOADTYPE = 0,
                .CHOICE_NAME = VEC_EMPTY,
                .CHOICE_VAL = VEC_EMPTY,
                .DEFAULT_VAL = 0,
            };
            VEC_PUSH(ty.itemdef, def, arena);
        }
        {
            DBData data = {
                .name = SV(""),
                .values = VEC_EMPTY,
            };
            VEC_PUSH(data.values, (DBVal){.str_val = SV("")}, arena);
            VEC_PUSH(ty.data, data, arena);
        }
        VEC_PUSH(db, ty, arena);
    }
}

#endif // WOD_SDB_H_
