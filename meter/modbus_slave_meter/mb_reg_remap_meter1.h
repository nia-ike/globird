


/*****************第1回路寄存器段，开始*****************************************************************************/

/* 
    meter1 表工作信息  (9CT型号等于 GRID； 6CT时需要根据配置和现场接线会另外映射出GRID，因此跟当前的不一致)
    包括参数、版本，用于给用户系统进行基础数据判断
*/
struct remap_Modbus_node remapInfo_meter1[] =  {
    REG_READ_ALIGN_U16   (db_cfg_info.device.SoftVersion                                        , REG_ORDER_LITTLE_EDIAN  ), // 0
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.user.base.Addr                               , REG_ORDER_LITTLE_EDIAN  ), // 1
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_cfg.user.base.BaudRate                           , REG_ORDER_LITTLE_EDIAN  ), // 2
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_cfg.user.base.BaudRate                           , REG_ORDER_LITTLE_EDIAN  ), // 3
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.user.base.CharSize                           , REG_ORDER_LITTLE_EDIAN  ), // 4
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.user.base.ParityBit                          , REG_ORDER_LITTLE_EDIAN  ), // 5
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.user.base.StopBit                            , REG_ORDER_LITTLE_EDIAN  ), // 6
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.user.base.WiringMode                         , REG_ORDER_LITTLE_EDIAN  ), // 7
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_cfg.user.base.PT                                 , REG_ORDER_LITTLE_EDIAN  ), // 8
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_cfg.user.base.PT                                 , REG_ORDER_LITTLE_EDIAN  ), // 9
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_cfg.user.base.CT                                 , REG_ORDER_LITTLE_EDIAN  ), // 10
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_cfg.user.base.CT                                 , REG_ORDER_LITTLE_EDIAN  ), // 11
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.user.base.PhaseAdjust1                       , REG_ORDER_LITTLE_EDIAN  ), // 12
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.user.base.PhaseAdjust2                       , REG_ORDER_LITTLE_EDIAN  ), // 13
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.user.base.PhaseAdjust3                       , REG_ORDER_LITTLE_EDIAN  ), // 14
    REG_READ_ALIGN_U16   (db_cfg_info.device.device[0]                                          , REG_ORDER_BIG_EDIAN     ), // 15
    REG_READ_ALIGN_U16   (db_cfg_info.device.device[2]                                          , REG_ORDER_BIG_EDIAN     ), // 16
    REG_READ_ALIGN_U16   (db_cfg_info.device.device[4]                                          , REG_ORDER_BIG_EDIAN     ), // 17
    REG_READ_ALIGN_U16   (db_cfg_info.device.device[6]                                          , REG_ORDER_BIG_EDIAN     ), // 18
    REG_READ_ALIGN_U16   (db_cfg_info.device.device[8]                                          , REG_ORDER_BIG_EDIAN     ), // 19
    REG_READ_ALIGN_U16   (db_cfg_info.device.device[10]                                         , REG_ORDER_BIG_EDIAN     ), // 20
    REG_READ_ALIGN_U16   (db_cfg_info.device.device[12]                                         , REG_ORDER_BIG_EDIAN     ), // 21
    REG_READ_ALIGN_U16   (db_cfg_info.device.device[14]                                         , REG_ORDER_BIG_EDIAN     ), // 22
    REG_READ_ALIGN_U16   (db_cfg_info.device.SNInfo[0]                                          , REG_ORDER_BIG_EDIAN     ), // 23
    REG_READ_ALIGN_U16   (db_cfg_info.device.SNInfo[2]                                          , REG_ORDER_BIG_EDIAN     ), // 24
    REG_READ_ALIGN_U16   (db_cfg_info.device.SNInfo[4]                                          , REG_ORDER_BIG_EDIAN     ), // 25
    REG_READ_ALIGN_U16   (db_cfg_info.device.SNInfo[6]                                          , REG_ORDER_BIG_EDIAN     ), // 26
    REG_READ_ALIGN_U16   (db_cfg_info.device.SNInfo[8]                                          , REG_ORDER_BIG_EDIAN     ), // 27
    REG_READ_ALIGN_U16   (db_cfg_info.device.SNInfo[10]                                         , REG_ORDER_BIG_EDIAN     ), // 28
    REG_READ_ALIGN_U16   (db_cfg_info.device.SNInfo[12]                                         , REG_ORDER_BIG_EDIAN     ), // 29
    REG_READ_ALIGN_U16   (db_cfg_info.device.SNInfo[14]                                         , REG_ORDER_BIG_EDIAN     ), // 30
    REG_READ_ALIGN_U16   (db_cfg_info.device.SNInfo[16]                                         , REG_ORDER_BIG_EDIAN     ), // 31
    REG_READ_ALIGN_U16   (db_cfg_info.device.SNInfo[18]                                         , REG_ORDER_BIG_EDIAN     ), // 32
    REG_READ_ALIGN_U16   (db_cfg_info.device.SNInfo[20]                                         , REG_ORDER_BIG_EDIAN     ), // 33
    REG_READ_ALIGN_U16   (db_cfg_info.device.SNInfo[22]                                         , REG_ORDER_BIG_EDIAN     ), // 34
    REG_READ_ALIGN_U16   (db_cfg_info.device.SNInfo[24]                                         , REG_ORDER_BIG_EDIAN     ), // 35
};

/* 
    数据寄存器聚合 （放的是快速刷新数据）
    用于把常规电参量数据连续寄存器能够读全，尽量少分帧
*/
struct remap_Modbus_node remapAggregation_meter1[] =  {
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.halfwave.primary.Va              , REG_ORDER_LITTLE_EDIAN  ), // 0
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.halfwave.primary.Va              , REG_ORDER_LITTLE_EDIAN  ), // 1
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.halfwave.primary.Vb              , REG_ORDER_LITTLE_EDIAN  ), // 2
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.halfwave.primary.Vb              , REG_ORDER_LITTLE_EDIAN  ), // 3
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.halfwave.primary.Vc              , REG_ORDER_LITTLE_EDIAN  ), // 4
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.halfwave.primary.Vc              , REG_ORDER_LITTLE_EDIAN  ), // 5
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.halfwave.primary.VlnAvg          , REG_ORDER_LITTLE_EDIAN  ), // 6
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.halfwave.primary.VlnAvg          , REG_ORDER_LITTLE_EDIAN  ), // 7
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.halfwave.primary.Vab             , REG_ORDER_LITTLE_EDIAN  ), // 8
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.halfwave.primary.Vab             , REG_ORDER_LITTLE_EDIAN  ), // 9
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.halfwave.primary.Vbc             , REG_ORDER_LITTLE_EDIAN  ), // 10
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.halfwave.primary.Vbc             , REG_ORDER_LITTLE_EDIAN  ), // 11
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.halfwave.primary.Vca             , REG_ORDER_LITTLE_EDIAN  ), // 12
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.halfwave.primary.Vca             , REG_ORDER_LITTLE_EDIAN  ), // 13
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.halfwave.primary.VllAvg          , REG_ORDER_LITTLE_EDIAN  ), // 14
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.halfwave.primary.VllAvg          , REG_ORDER_LITTLE_EDIAN  ), // 15
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.halfwave.primary.Ia              , REG_ORDER_LITTLE_EDIAN  ), // 16
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.halfwave.primary.Ia              , REG_ORDER_LITTLE_EDIAN  ), // 17
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.halfwave.primary.Ib              , REG_ORDER_LITTLE_EDIAN  ), // 18
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.halfwave.primary.Ib              , REG_ORDER_LITTLE_EDIAN  ), // 19
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.halfwave.primary.Ic              , REG_ORDER_LITTLE_EDIAN  ), // 20
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.halfwave.primary.Ic              , REG_ORDER_LITTLE_EDIAN  ), // 21
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.halfwave.primary.IAvg            , REG_ORDER_LITTLE_EDIAN  ), // 22
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.halfwave.primary.IAvg            , REG_ORDER_LITTLE_EDIAN  ), // 23
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.halfwave.primary.In              , REG_ORDER_LITTLE_EDIAN  ), // 24
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.halfwave.primary.In              , REG_ORDER_LITTLE_EDIAN  ), // 25
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.halfwave.primary.Pa              , REG_ORDER_LITTLE_EDIAN  ), // 26  
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.halfwave.primary.Pa              , REG_ORDER_LITTLE_EDIAN  ), // 27
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.halfwave.primary.Pb              , REG_ORDER_LITTLE_EDIAN  ), // 28
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.halfwave.primary.Pb              , REG_ORDER_LITTLE_EDIAN  ), // 29
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.halfwave.primary.Pc              , REG_ORDER_LITTLE_EDIAN  ), // 30
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.halfwave.primary.Pc              , REG_ORDER_LITTLE_EDIAN  ), // 31
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.halfwave.primary.Qa              , REG_ORDER_LITTLE_EDIAN  ), // 32
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.halfwave.primary.Qa              , REG_ORDER_LITTLE_EDIAN  ), // 33
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.halfwave.primary.Qb              , REG_ORDER_LITTLE_EDIAN  ), // 34
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.halfwave.primary.Qb              , REG_ORDER_LITTLE_EDIAN  ), // 35
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.halfwave.primary.Qc              , REG_ORDER_LITTLE_EDIAN  ), // 36
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.halfwave.primary.Qc              , REG_ORDER_LITTLE_EDIAN  ), // 37
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.halfwave.primary.Sa              , REG_ORDER_LITTLE_EDIAN  ), // 38
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.halfwave.primary.Sa              , REG_ORDER_LITTLE_EDIAN  ), // 39
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.halfwave.primary.Sb              , REG_ORDER_LITTLE_EDIAN  ), // 40
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.halfwave.primary.Sb              , REG_ORDER_LITTLE_EDIAN  ), // 41
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.halfwave.primary.Sc              , REG_ORDER_LITTLE_EDIAN  ), // 42
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.halfwave.primary.Sc              , REG_ORDER_LITTLE_EDIAN  ), // 43
    REG_READ_ALIGN_U16   (db_measurebase[0].db_measure.numeric.halfwave.primary.PFa             , REG_ORDER_LITTLE_EDIAN  ), // 44
    REG_READ_ALIGN_U16   (db_measurebase[0].db_measure.numeric.halfwave.primary.PFb             , REG_ORDER_LITTLE_EDIAN  ), // 45
    REG_READ_ALIGN_U16   (db_measurebase[0].db_measure.numeric.halfwave.primary.PFc             , REG_ORDER_LITTLE_EDIAN  ), // 46
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.halfwave.primary.Ptot            , REG_ORDER_LITTLE_EDIAN  ), // 47
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.halfwave.primary.Ptot            , REG_ORDER_LITTLE_EDIAN  ), // 48
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.halfwave.primary.Qtot            , REG_ORDER_LITTLE_EDIAN  ), // 49
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.halfwave.primary.Qtot            , REG_ORDER_LITTLE_EDIAN  ), // 50
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.halfwave.primary.Stot            , REG_ORDER_LITTLE_EDIAN  ), // 51
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.halfwave.primary.Stot            , REG_ORDER_LITTLE_EDIAN  ), // 52
    REG_READ_ALIGN_U16   (db_measurebase[0].db_measure.numeric.halfwave.primary.PFtot           , REG_ORDER_LITTLE_EDIAN  ), // 53
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.halfwave.primary.Hz_hp           , REG_ORDER_LITTLE_EDIAN  ), // 54
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.halfwave.primary.Hz_hp           , REG_ORDER_LITTLE_EDIAN  ), // 55
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.halfwave.primary.J_Va            , REG_ORDER_LITTLE_EDIAN  ), // 56
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.halfwave.primary.J_Vb            , REG_ORDER_LITTLE_EDIAN  ), // 57
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.halfwave.primary.J_Vc            , REG_ORDER_LITTLE_EDIAN  ), // 58
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.halfwave.primary.J_Ia            , REG_ORDER_LITTLE_EDIAN  ), // 59
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.halfwave.primary.J_Ib            , REG_ORDER_LITTLE_EDIAN  ), // 60
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.halfwave.primary.J_Ic            , REG_ORDER_LITTLE_EDIAN  ), // 61
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.energy.realtime.kWhImp[0]                , REG_ORDER_LITTLE_EDIAN  ), // 62
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.energy.realtime.kWhImp[0]                , REG_ORDER_LITTLE_EDIAN  ), // 63
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.energy.realtime.kWhImp[1]                , REG_ORDER_LITTLE_EDIAN  ), // 64
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.energy.realtime.kWhImp[1]                , REG_ORDER_LITTLE_EDIAN  ), // 65
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.energy.realtime.kWhImp[2]                , REG_ORDER_LITTLE_EDIAN  ), // 66
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.energy.realtime.kWhImp[2]                , REG_ORDER_LITTLE_EDIAN  ), // 67
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.energy.realtime.kWhImp[3]                , REG_ORDER_LITTLE_EDIAN  ), // 68
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.energy.realtime.kWhImp[3]                , REG_ORDER_LITTLE_EDIAN  ), // 69
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.energy.realtime.kWhExp[0]                , REG_ORDER_LITTLE_EDIAN  ), // 70
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.energy.realtime.kWhExp[0]                , REG_ORDER_LITTLE_EDIAN  ), // 71
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.energy.realtime.kWhExp[1]                , REG_ORDER_LITTLE_EDIAN  ), // 72
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.energy.realtime.kWhExp[1]                , REG_ORDER_LITTLE_EDIAN  ), // 73
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.energy.realtime.kWhExp[2]                , REG_ORDER_LITTLE_EDIAN  ), // 74
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.energy.realtime.kWhExp[2]                , REG_ORDER_LITTLE_EDIAN  ), // 75
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.energy.realtime.kWhExp[3]                , REG_ORDER_LITTLE_EDIAN  ), // 76
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.energy.realtime.kWhExp[3]                , REG_ORDER_LITTLE_EDIAN  ), // 77
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.statistical.demand.realtime.Ptot         , REG_ORDER_LITTLE_EDIAN  ), // 78
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.statistical.demand.realtime.Ptot         , REG_ORDER_LITTLE_EDIAN  ), // 79
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.statistical.demand.realtime.Ptot_neg         , REG_ORDER_LITTLE_EDIAN  ), // 80
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.statistical.demand.realtime.Ptot_neg         , REG_ORDER_LITTLE_EDIAN  ), // 81
};


/* 
    基波分段
*/
struct remap_Modbus_node  remapRealTimeFundwave_meter1[] = {
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.fundwave.primary.Va              , REG_ORDER_LITTLE_EDIAN  ), // 0
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.fundwave.primary.Va              , REG_ORDER_LITTLE_EDIAN  ), // 1
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.fundwave.primary.Vb              , REG_ORDER_LITTLE_EDIAN  ), // 2
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.fundwave.primary.Vb              , REG_ORDER_LITTLE_EDIAN  ), // 3
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.fundwave.primary.Vc              , REG_ORDER_LITTLE_EDIAN  ), // 4
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.fundwave.primary.Vc              , REG_ORDER_LITTLE_EDIAN  ), // 5
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.fundwave.primary.VlnAvg          , REG_ORDER_LITTLE_EDIAN  ), // 6
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.fundwave.primary.VlnAvg          , REG_ORDER_LITTLE_EDIAN  ), // 7
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.fundwave.primary.Vab             , REG_ORDER_LITTLE_EDIAN  ), // 8
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.fundwave.primary.Vab             , REG_ORDER_LITTLE_EDIAN  ), // 9
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.fundwave.primary.Vbc             , REG_ORDER_LITTLE_EDIAN  ), // 10
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.fundwave.primary.Vbc             , REG_ORDER_LITTLE_EDIAN  ), // 11
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.fundwave.primary.Vca             , REG_ORDER_LITTLE_EDIAN  ), // 12
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.fundwave.primary.Vca             , REG_ORDER_LITTLE_EDIAN  ), // 13
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.fundwave.primary.VllAvg          , REG_ORDER_LITTLE_EDIAN  ), // 14
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.fundwave.primary.VllAvg          , REG_ORDER_LITTLE_EDIAN  ), // 15
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.fundwave.primary.Ia              , REG_ORDER_LITTLE_EDIAN  ), // 16
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.fundwave.primary.Ia              , REG_ORDER_LITTLE_EDIAN  ), // 17
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.fundwave.primary.Ib              , REG_ORDER_LITTLE_EDIAN  ), // 18
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.fundwave.primary.Ib              , REG_ORDER_LITTLE_EDIAN  ), // 19
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.fundwave.primary.Ic              , REG_ORDER_LITTLE_EDIAN  ), // 20
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.fundwave.primary.Ic              , REG_ORDER_LITTLE_EDIAN  ), // 21
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.fundwave.primary.IAvg            , REG_ORDER_LITTLE_EDIAN  ), // 22
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.fundwave.primary.IAvg            , REG_ORDER_LITTLE_EDIAN  ), // 23
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.fundwave.primary.In              , REG_ORDER_LITTLE_EDIAN  ), // 24
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.fundwave.primary.In              , REG_ORDER_LITTLE_EDIAN  ), // 25
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.fundwave.primary.Pa              , REG_ORDER_LITTLE_EDIAN  ), // 26
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.fundwave.primary.Pa              , REG_ORDER_LITTLE_EDIAN  ), // 27
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.fundwave.primary.Pb              , REG_ORDER_LITTLE_EDIAN  ), // 28
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.fundwave.primary.Pb              , REG_ORDER_LITTLE_EDIAN  ), // 29
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.fundwave.primary.Pc              , REG_ORDER_LITTLE_EDIAN  ), // 30
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.fundwave.primary.Pc              , REG_ORDER_LITTLE_EDIAN  ), // 31
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.fundwave.primary.Qa              , REG_ORDER_LITTLE_EDIAN  ), // 32
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.fundwave.primary.Qa              , REG_ORDER_LITTLE_EDIAN  ), // 33
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.fundwave.primary.Qb              , REG_ORDER_LITTLE_EDIAN  ), // 34
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.fundwave.primary.Qb              , REG_ORDER_LITTLE_EDIAN  ), // 35
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.fundwave.primary.Qc              , REG_ORDER_LITTLE_EDIAN  ), // 36
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.fundwave.primary.Qc              , REG_ORDER_LITTLE_EDIAN  ), // 37
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.fundwave.primary.Sa              , REG_ORDER_LITTLE_EDIAN  ), // 38
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.fundwave.primary.Sa              , REG_ORDER_LITTLE_EDIAN  ), // 39
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.fundwave.primary.Sb              , REG_ORDER_LITTLE_EDIAN  ), // 40
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.fundwave.primary.Sb              , REG_ORDER_LITTLE_EDIAN  ), // 41
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.fundwave.primary.Sc              , REG_ORDER_LITTLE_EDIAN  ), // 42
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.fundwave.primary.Sc              , REG_ORDER_LITTLE_EDIAN  ), // 43
    REG_READ_ALIGN_U16   (db_measurebase[0].db_measure.numeric.fundwave.primary.PFa             , REG_ORDER_LITTLE_EDIAN  ), // 44
    REG_READ_ALIGN_U16   (db_measurebase[0].db_measure.numeric.fundwave.primary.PFb             , REG_ORDER_LITTLE_EDIAN  ), // 45
    REG_READ_ALIGN_U16   (db_measurebase[0].db_measure.numeric.fundwave.primary.PFc             , REG_ORDER_LITTLE_EDIAN  ), // 46
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.fundwave.primary.Ptot            , REG_ORDER_LITTLE_EDIAN  ), // 47
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.fundwave.primary.Ptot            , REG_ORDER_LITTLE_EDIAN  ), // 48
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.fundwave.primary.Qtot            , REG_ORDER_LITTLE_EDIAN  ), // 49
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.fundwave.primary.Qtot            , REG_ORDER_LITTLE_EDIAN  ), // 50
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.fundwave.primary.Stot            , REG_ORDER_LITTLE_EDIAN  ), // 51
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.fundwave.primary.Stot            , REG_ORDER_LITTLE_EDIAN  ), // 52
    REG_READ_ALIGN_U16   (db_measurebase[0].db_measure.numeric.fundwave.primary.PFtot           , REG_ORDER_LITTLE_EDIAN  ), // 53
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.fundwave.primary.Hz_hp           , REG_ORDER_LITTLE_EDIAN  ), // 54
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.fundwave.primary.Hz_hp           , REG_ORDER_LITTLE_EDIAN  ), // 55
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.fundwave.primary.Pab             , REG_ORDER_LITTLE_EDIAN  ), // 56
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.fundwave.primary.Pab             , REG_ORDER_LITTLE_EDIAN  ), // 57
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.fundwave.primary.Pca             , REG_ORDER_LITTLE_EDIAN  ), // 58
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.fundwave.primary.Pca             , REG_ORDER_LITTLE_EDIAN  ), // 59
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.fundwave.primary.Qab             , REG_ORDER_LITTLE_EDIAN  ), // 60
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.fundwave.primary.Qab             , REG_ORDER_LITTLE_EDIAN  ), // 61
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.fundwave.primary.Qca             , REG_ORDER_LITTLE_EDIAN  ), // 62
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.fundwave.primary.Qca             , REG_ORDER_LITTLE_EDIAN  ), // 63
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.fundwave.primary.Sab             , REG_ORDER_LITTLE_EDIAN  ), // 64
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.fundwave.primary.Sab             , REG_ORDER_LITTLE_EDIAN  ), // 65
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.fundwave.primary.Sca             , REG_ORDER_LITTLE_EDIAN  ), // 66
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.fundwave.primary.Sca             , REG_ORDER_LITTLE_EDIAN  ), // 67

};


/*
   以下为电表测量的常规数据，比聚合寄存器要全。刷新为慢速刷新
*/
struct remap_Modbus_node remapSeg_realtime_meter1[] = {
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.fullwave.primary.Va              , REG_ORDER_LITTLE_EDIAN  ), // 0
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.fullwave.primary.Va              , REG_ORDER_LITTLE_EDIAN  ), // 1
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.fullwave.primary.Vb              , REG_ORDER_LITTLE_EDIAN  ), // 2
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.fullwave.primary.Vb              , REG_ORDER_LITTLE_EDIAN  ), // 3
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.fullwave.primary.Vc              , REG_ORDER_LITTLE_EDIAN  ), // 4
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.fullwave.primary.Vc              , REG_ORDER_LITTLE_EDIAN  ), // 5
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.fullwave.primary.VlnAvg          , REG_ORDER_LITTLE_EDIAN  ), // 6
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.fullwave.primary.VlnAvg          , REG_ORDER_LITTLE_EDIAN  ), // 7
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.fullwave.primary.Vab             , REG_ORDER_LITTLE_EDIAN  ), // 8
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.fullwave.primary.Vab             , REG_ORDER_LITTLE_EDIAN  ), // 9
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.fullwave.primary.Vbc             , REG_ORDER_LITTLE_EDIAN  ), // 10
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.fullwave.primary.Vbc             , REG_ORDER_LITTLE_EDIAN  ), // 11
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.fullwave.primary.Vca             , REG_ORDER_LITTLE_EDIAN  ), // 12
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.fullwave.primary.Vca             , REG_ORDER_LITTLE_EDIAN  ), // 13
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.fullwave.primary.VllAvg          , REG_ORDER_LITTLE_EDIAN  ), // 14
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.fullwave.primary.VllAvg          , REG_ORDER_LITTLE_EDIAN  ), // 15
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.fullwave.primary.Ia              , REG_ORDER_LITTLE_EDIAN  ), // 16
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.fullwave.primary.Ia              , REG_ORDER_LITTLE_EDIAN  ), // 17
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.fullwave.primary.Ib              , REG_ORDER_LITTLE_EDIAN  ), // 18
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.fullwave.primary.Ib              , REG_ORDER_LITTLE_EDIAN  ), // 19
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.fullwave.primary.Ic              , REG_ORDER_LITTLE_EDIAN  ), // 20
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.fullwave.primary.Ic              , REG_ORDER_LITTLE_EDIAN  ), // 21
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.fullwave.primary.IAvg            , REG_ORDER_LITTLE_EDIAN  ), // 22
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.fullwave.primary.IAvg            , REG_ORDER_LITTLE_EDIAN  ), // 23
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.fullwave.primary.In              , REG_ORDER_LITTLE_EDIAN  ), // 24
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.fullwave.primary.In              , REG_ORDER_LITTLE_EDIAN  ), // 25
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.fullwave.primary.Pa              , REG_ORDER_LITTLE_EDIAN  ), // 26  26  ABC相功率变量需要32位， 后面2个dummy寄存器用于预留
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.fullwave.primary.Pa              , REG_ORDER_LITTLE_EDIAN  ), // 27
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.fullwave.primary.Pb              , REG_ORDER_LITTLE_EDIAN  ), // 28
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.fullwave.primary.Pb              , REG_ORDER_LITTLE_EDIAN  ), // 29
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.fullwave.primary.Pc              , REG_ORDER_LITTLE_EDIAN  ), // 30
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.fullwave.primary.Pc              , REG_ORDER_LITTLE_EDIAN  ), // 31
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.fullwave.primary.Qa              , REG_ORDER_LITTLE_EDIAN  ), // 32
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.fullwave.primary.Qa              , REG_ORDER_LITTLE_EDIAN  ), // 33
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.fullwave.primary.Qb              , REG_ORDER_LITTLE_EDIAN  ), // 34
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.fullwave.primary.Qb              , REG_ORDER_LITTLE_EDIAN  ), // 35
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.fullwave.primary.Qc              , REG_ORDER_LITTLE_EDIAN  ), // 36
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.fullwave.primary.Qc              , REG_ORDER_LITTLE_EDIAN  ), // 37
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.fullwave.primary.Sa              , REG_ORDER_LITTLE_EDIAN  ), // 38
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.fullwave.primary.Sa              , REG_ORDER_LITTLE_EDIAN  ), // 39
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.fullwave.primary.Sb              , REG_ORDER_LITTLE_EDIAN  ), // 40
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.fullwave.primary.Sb              , REG_ORDER_LITTLE_EDIAN  ), // 41
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.fullwave.primary.Sc              , REG_ORDER_LITTLE_EDIAN  ), // 42
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.fullwave.primary.Sc              , REG_ORDER_LITTLE_EDIAN  ), // 43
    REG_READ_ALIGN_U16   (db_measurebase[0].db_measure.numeric.fullwave.primary.PFa             , REG_ORDER_LITTLE_EDIAN  ), // 44
    REG_READ_ALIGN_U16   (db_measurebase[0].db_measure.numeric.fullwave.primary.PFb             , REG_ORDER_LITTLE_EDIAN  ), // 45
    REG_READ_ALIGN_U16   (db_measurebase[0].db_measure.numeric.fullwave.primary.PFc             , REG_ORDER_LITTLE_EDIAN  ), // 46
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.fullwave.primary.Ptot            , REG_ORDER_LITTLE_EDIAN  ), // 47
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.fullwave.primary.Ptot            , REG_ORDER_LITTLE_EDIAN  ), // 48
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.fullwave.primary.Qtot            , REG_ORDER_LITTLE_EDIAN  ), // 49
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.fullwave.primary.Qtot            , REG_ORDER_LITTLE_EDIAN  ), // 50
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.fullwave.primary.Stot            , REG_ORDER_LITTLE_EDIAN  ), // 51
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.fullwave.primary.Stot            , REG_ORDER_LITTLE_EDIAN  ), // 52
    REG_READ_ALIGN_U16   (db_measurebase[0].db_measure.numeric.fullwave.primary.PFtot           , REG_ORDER_LITTLE_EDIAN  ), // 53
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.fullwave.primary.Hz_hp           , REG_ORDER_LITTLE_EDIAN  ), // 54
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.fullwave.primary.Hz_hp           , REG_ORDER_LITTLE_EDIAN  ), // 55
    REG_READ_ALIGN_U16   (db_measurebase[0].db_measure.numeric.fullwave.primary.J_Va            , REG_ORDER_LITTLE_EDIAN  ), // 56
    REG_READ_ALIGN_U16   (db_measurebase[0].db_measure.numeric.fullwave.primary.J_Vb            , REG_ORDER_LITTLE_EDIAN  ), // 57
    REG_READ_ALIGN_U16   (db_measurebase[0].db_measure.numeric.fullwave.primary.J_Vc            , REG_ORDER_LITTLE_EDIAN  ), // 58
    REG_READ_ALIGN_U16   (db_measurebase[0].db_measure.numeric.fullwave.primary.J_Ia            , REG_ORDER_LITTLE_EDIAN  ), // 59
    REG_READ_ALIGN_U16   (db_measurebase[0].db_measure.numeric.fullwave.primary.J_Ib            , REG_ORDER_LITTLE_EDIAN  ), // 60
    REG_READ_ALIGN_U16   (db_measurebase[0].db_measure.numeric.fullwave.primary.J_Ic            , REG_ORDER_LITTLE_EDIAN  ), // 61
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.powerquality.basic.unbalance.Unbalance_V.UnbalanceValue.S0, REG_ORDER_LITTLE_EDIAN  ), // 62
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.powerquality.basic.unbalance.Unbalance_V.UnbalanceValue.S0, REG_ORDER_LITTLE_EDIAN  ), // 63
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.powerquality.basic.unbalance.Unbalance_V.UnbalanceValue.S1, REG_ORDER_LITTLE_EDIAN  ), // 64
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.powerquality.basic.unbalance.Unbalance_V.UnbalanceValue.S1, REG_ORDER_LITTLE_EDIAN  ), // 65
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.powerquality.basic.unbalance.Unbalance_V.UnbalanceValue.S2, REG_ORDER_LITTLE_EDIAN  ), // 66
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.powerquality.basic.unbalance.Unbalance_V.UnbalanceValue.S2, REG_ORDER_LITTLE_EDIAN  ), // 67
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.powerquality.basic.unbalance.Unbalance_I.UnbalanceValue.S0, REG_ORDER_LITTLE_EDIAN  ), // 68
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.powerquality.basic.unbalance.Unbalance_I.UnbalanceValue.S0, REG_ORDER_LITTLE_EDIAN  ), // 69
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.powerquality.basic.unbalance.Unbalance_I.UnbalanceValue.S1, REG_ORDER_LITTLE_EDIAN  ), // 70
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.powerquality.basic.unbalance.Unbalance_I.UnbalanceValue.S1, REG_ORDER_LITTLE_EDIAN  ), // 71
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.powerquality.basic.unbalance.Unbalance_I.UnbalanceValue.S2, REG_ORDER_LITTLE_EDIAN  ), // 72
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.powerquality.basic.unbalance.Unbalance_I.UnbalanceValue.S2, REG_ORDER_LITTLE_EDIAN  ), // 73
    REG_READ_ALIGN_U16   (db_measurebase[0].db_measure.powerquality.basic.unbalance.Unbalance_V.UnbalanceFactor.I0, REG_ORDER_LITTLE_EDIAN  ), // 74
    REG_READ_ALIGN_U16   (db_measurebase[0].db_measure.powerquality.basic.unbalance.Unbalance_V.UnbalanceFactor.I1, REG_ORDER_LITTLE_EDIAN  ), // 75
    REG_READ_ALIGN_U16   (db_measurebase[0].db_measure.powerquality.basic.unbalance.Unbalance_I.UnbalanceFactor.I0, REG_ORDER_LITTLE_EDIAN  ), // 76
    REG_READ_ALIGN_U16   (db_measurebase[0].db_measure.powerquality.basic.unbalance.Unbalance_I.UnbalanceFactor.I1, REG_ORDER_LITTLE_EDIAN  ), // 77
    REG_READ_ALIGN_U16   (db_null.dummy, REG_ORDER_LITTLE_EDIAN  ), // 78
    REG_READ_ALIGN_U16   (db_null.dummy, REG_ORDER_LITTLE_EDIAN  ), // 79
    REG_READ_ALIGN_U16   (db_null.dummy, REG_ORDER_LITTLE_EDIAN  ), // 80
    REG_READ_ALIGN_U16   (db_null.dummy, REG_ORDER_LITTLE_EDIAN  ), // 81
    REG_READ_ALIGN_U16   (db_null.dummy, REG_ORDER_LITTLE_EDIAN  ), // 82
    REG_READ_ALIGN_U16   (db_null.dummy, REG_ORDER_LITTLE_EDIAN  ), // 83
    REG_READ_ALIGN_U16   (db_null.dummy, REG_ORDER_LITTLE_EDIAN  ), // 84
    REG_READ_ALIGN_U16   (db_measurebase[0].db_measure.numeric.fullwave.primary.DemoFlag        , REG_ORDER_LITTLE_EDIAN  ), // 85
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.fullwave.primary.Pab             , REG_ORDER_LITTLE_EDIAN  ), // 86
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.fullwave.primary.Pab             , REG_ORDER_LITTLE_EDIAN  ), // 87
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.fullwave.primary.Pca             , REG_ORDER_LITTLE_EDIAN  ), // 88
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.fullwave.primary.Pca             , REG_ORDER_LITTLE_EDIAN  ), // 89
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.fullwave.primary.Qab             , REG_ORDER_LITTLE_EDIAN  ), // 90
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.fullwave.primary.Qab             , REG_ORDER_LITTLE_EDIAN  ), // 91
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.fullwave.primary.Qca             , REG_ORDER_LITTLE_EDIAN  ), // 92
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.fullwave.primary.Qca             , REG_ORDER_LITTLE_EDIAN  ), // 93
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.fullwave.primary.Sab             , REG_ORDER_LITTLE_EDIAN  ), // 94
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.fullwave.primary.Sab             , REG_ORDER_LITTLE_EDIAN  ), // 95
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.numeric.fullwave.primary.Sca             , REG_ORDER_LITTLE_EDIAN  ), // 96
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.numeric.fullwave.primary.Sca             , REG_ORDER_LITTLE_EDIAN  ), // 97
    REG_READ_ALIGN_U16   (db_measurebase[0].db_measure.powerquality.basic.OffsetVa              , REG_ORDER_LITTLE_EDIAN  ), // 98
    REG_READ_ALIGN_U16   (db_measurebase[0].db_measure.powerquality.basic.OffsetVb              , REG_ORDER_LITTLE_EDIAN  ), // 99
    REG_READ_ALIGN_U16   (db_measurebase[0].db_measure.powerquality.basic.OffsetVc              , REG_ORDER_LITTLE_EDIAN  ), // 100
    REG_READ_ALIGN_U16   (db_measurebase[0].db_measure.powerquality.basic.OffsetVab             , REG_ORDER_LITTLE_EDIAN  ), // 101
    REG_READ_ALIGN_U16   (db_measurebase[0].db_measure.powerquality.basic.OffsetVbc             , REG_ORDER_LITTLE_EDIAN  ), // 102
    REG_READ_ALIGN_U16   (db_measurebase[0].db_measure.powerquality.basic.OffsetVca             , REG_ORDER_LITTLE_EDIAN  ), // 103
    REG_READ_ALIGN_U16   (db_measurebase[0].db_measure.powerquality.basic.OffsetHz              , REG_ORDER_LITTLE_EDIAN  ), // 104
    REG_READ_ALIGN_U16   (db_measurebase[0].db_measure.powerquality.basic.loadrate              , REG_ORDER_LITTLE_EDIAN  ), // 105
    REG_READ_ALIGN_U16   (db_measurebase[0].db_measure.powerquality.basic.WaveMaxFactorUa       , REG_ORDER_LITTLE_EDIAN  ), // 106
    REG_READ_ALIGN_U16   (db_measurebase[0].db_measure.powerquality.basic.WaveMaxFactorUb       , REG_ORDER_LITTLE_EDIAN  ), // 107
    REG_READ_ALIGN_U16   (db_measurebase[0].db_measure.powerquality.basic.WaveMaxFactorUc       , REG_ORDER_LITTLE_EDIAN  ), // 108
    REG_READ_ALIGN_U16   (db_measurebase[0].db_measure.powerquality.basic.WaveMaxFactorIa       , REG_ORDER_LITTLE_EDIAN  ), // 109
    REG_READ_ALIGN_U16   (db_measurebase[0].db_measure.powerquality.basic.WaveMaxFactorIb       , REG_ORDER_LITTLE_EDIAN  ), // 110
    REG_READ_ALIGN_U16   (db_measurebase[0].db_measure.powerquality.basic.WaveMaxFactorIc       , REG_ORDER_LITTLE_EDIAN  ), // 111
    REG_READ_ALIGN_U16   (db_measurebase[0].db_measure.powerquality.basic.TDDIa                 , REG_ORDER_LITTLE_EDIAN  ), // 112
    REG_READ_ALIGN_U16   (db_measurebase[0].db_measure.powerquality.basic.TDDIb                 , REG_ORDER_LITTLE_EDIAN  ), // 113
    REG_READ_ALIGN_U16   (db_measurebase[0].db_measure.powerquality.basic.TDDIc                 , REG_ORDER_LITTLE_EDIAN  ), // 114
    REG_READ_ALIGN_U16   (db_measurebase[0].db_measure.powerquality.basic.KFactorIa             , REG_ORDER_LITTLE_EDIAN  ), // 115
    REG_READ_ALIGN_U16   (db_measurebase[0].db_measure.powerquality.basic.KFactorIb             , REG_ORDER_LITTLE_EDIAN  ), // 116
    REG_READ_ALIGN_U16   (db_measurebase[0].db_measure.powerquality.basic.KFactorIc             , REG_ORDER_LITTLE_EDIAN  ), // 117

    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.statistical.loadrun.lrh                  , REG_ORDER_LITTLE_EDIAN  ), // 118
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.statistical.loadrun.lrh                  , REG_ORDER_LITTLE_EDIAN  ), // 119
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.statistical.loadrun.loh                  , REG_ORDER_LITTLE_EDIAN  ), // 120
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.statistical.loadrun.loh                  , REG_ORDER_LITTLE_EDIAN  ), // 121
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.statistical.loadrun.lrc                  , REG_ORDER_LITTLE_EDIAN  ), // 122
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.statistical.loadrun.lrc                  , REG_ORDER_LITTLE_EDIAN  ), // 123
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.statistical.loadrun.doh                  , REG_ORDER_LITTLE_EDIAN  ), // 124
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.statistical.loadrun.doh                  , REG_ORDER_LITTLE_EDIAN  ), // 125
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.statistical.loadrun.doh_uptime           , REG_ORDER_LITTLE_EDIAN  ), // 126
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.statistical.loadrun.doh_uptime           , REG_ORDER_LITTLE_EDIAN  ), // 127

   
};


/* 
    实时电度 
    由于所有电能已经从64位改成32位。 最大翻转电度为100W量程
    因此不需要再分1位小数的电能。直接输出3位小数电能
*/
struct remap_Modbus_node  remapRealTimeEnergyhp_meter1[] = {
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.energy.realtime_hp.kWhTot[3]             , REG_ORDER_LITTLE_EDIAN  ), // 0
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.energy.realtime_hp.kWhTot[3]             , REG_ORDER_LITTLE_EDIAN  ), // 1
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.energy.realtime_hp.kvarhTot[3]           , REG_ORDER_LITTLE_EDIAN  ), // 2
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.energy.realtime_hp.kvarhTot[3]           , REG_ORDER_LITTLE_EDIAN  ), // 3

    /* A */
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.energy.realtime_hp.kWhImp[0]             , REG_ORDER_LITTLE_EDIAN  ), // 4
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.energy.realtime_hp.kWhImp[0]             , REG_ORDER_LITTLE_EDIAN  ), // 5
     /* B */
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.energy.realtime_hp.kWhImp[1]             , REG_ORDER_LITTLE_EDIAN  ), // 6
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.energy.realtime_hp.kWhImp[1]             , REG_ORDER_LITTLE_EDIAN  ), // 7
     /* C */
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.energy.realtime_hp.kWhImp[2]             , REG_ORDER_LITTLE_EDIAN  ), // 8
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.energy.realtime_hp.kWhImp[2]             , REG_ORDER_LITTLE_EDIAN  ), // 9
    /* Tot */
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.energy.realtime_hp.kWhImp[3]             , REG_ORDER_LITTLE_EDIAN  ), // 10
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.energy.realtime_hp.kWhImp[3]             , REG_ORDER_LITTLE_EDIAN  ), // 11

    /* A */
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.energy.realtime_hp.kWhExp[0]             , REG_ORDER_LITTLE_EDIAN  ), // 12
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.energy.realtime_hp.kWhExp[0]             , REG_ORDER_LITTLE_EDIAN  ), // 13
     /* B */
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.energy.realtime_hp.kWhExp[1]             , REG_ORDER_LITTLE_EDIAN  ), // 14
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.energy.realtime_hp.kWhExp[1]             , REG_ORDER_LITTLE_EDIAN  ), // 15
     /* C */
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.energy.realtime_hp.kWhExp[2]             , REG_ORDER_LITTLE_EDIAN  ), // 16
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.energy.realtime_hp.kWhExp[2]             , REG_ORDER_LITTLE_EDIAN  ), // 17
    /* Tot */
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.energy.realtime_hp.kWhExp[3]             , REG_ORDER_LITTLE_EDIAN  ), // 18
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.energy.realtime_hp.kWhExp[3]             , REG_ORDER_LITTLE_EDIAN  ), // 19

    /* A */
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.energy.realtime_hp.kvarhImp[0]           , REG_ORDER_LITTLE_EDIAN  ), // 20
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.energy.realtime_hp.kvarhImp[0]           , REG_ORDER_LITTLE_EDIAN  ), // 21
     /* B */
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.energy.realtime_hp.kvarhImp[1]           , REG_ORDER_LITTLE_EDIAN  ), // 22
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.energy.realtime_hp.kvarhImp[1]           , REG_ORDER_LITTLE_EDIAN  ), // 23
     /* C */
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.energy.realtime_hp.kvarhImp[2]           , REG_ORDER_LITTLE_EDIAN  ), // 24
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.energy.realtime_hp.kvarhImp[2]           , REG_ORDER_LITTLE_EDIAN  ), // 25
    /* Tot */
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.energy.realtime_hp.kvarhImp[3]           , REG_ORDER_LITTLE_EDIAN  ), // 26
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.energy.realtime_hp.kvarhImp[3]           , REG_ORDER_LITTLE_EDIAN  ), // 27

    /* A */
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.energy.realtime_hp.kvarhExp[0]           , REG_ORDER_LITTLE_EDIAN  ), // 28
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.energy.realtime_hp.kvarhExp[0]           , REG_ORDER_LITTLE_EDIAN  ), // 29
     /* B */
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.energy.realtime_hp.kvarhExp[1]           , REG_ORDER_LITTLE_EDIAN  ), // 30
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.energy.realtime_hp.kvarhExp[1]           , REG_ORDER_LITTLE_EDIAN  ), // 31
     /* C */
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.energy.realtime_hp.kvarhExp[2]           , REG_ORDER_LITTLE_EDIAN  ), // 32
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.energy.realtime_hp.kvarhExp[2]           , REG_ORDER_LITTLE_EDIAN  ), // 33
    /* Tot */
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.energy.realtime_hp.kvarhExp[3]           , REG_ORDER_LITTLE_EDIAN  ), // 34
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.energy.realtime_hp.kvarhExp[3]           , REG_ORDER_LITTLE_EDIAN  ), // 35

    /* A */
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.energy.realtime_hp.kVAh[0]               , REG_ORDER_LITTLE_EDIAN  ), // 36
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.energy.realtime_hp.kVAh[0]               , REG_ORDER_LITTLE_EDIAN  ), // 37
     /* B */
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.energy.realtime_hp.kVAh[1]               , REG_ORDER_LITTLE_EDIAN  ), // 38
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.energy.realtime_hp.kVAh[1]               , REG_ORDER_LITTLE_EDIAN  ), // 39
     /* C */
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.energy.realtime_hp.kVAh[2]               , REG_ORDER_LITTLE_EDIAN  ), // 40
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.energy.realtime_hp.kVAh[2]               , REG_ORDER_LITTLE_EDIAN  ), // 41
    /* Tot */
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_measure.energy.realtime_hp.kVAh[3]               , REG_ORDER_LITTLE_EDIAN  ), // 42
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_measure.energy.realtime_hp.kVAh[3]               , REG_ORDER_LITTLE_EDIAN  ), // 43

};


struct remap_Modbus_node remapRealTimeDmd_meter1[] = {
    REG_READ_ALIGN_U32_CD( db_measurebase[0].db_measure.statistical.demand.realtime.Ia,             REG_ORDER_LITTLE_EDIAN), // 0
    REG_READ_ALIGN_U32_AB( db_measurebase[0].db_measure.statistical.demand.realtime.Ia,             REG_ORDER_LITTLE_EDIAN), // 1
    REG_READ_ALIGN_U32_CD( db_measurebase[0].db_measure.statistical.demand.realtime.Ib,             REG_ORDER_LITTLE_EDIAN), // 2
    REG_READ_ALIGN_U32_AB( db_measurebase[0].db_measure.statistical.demand.realtime.Ib,             REG_ORDER_LITTLE_EDIAN), // 3
    REG_READ_ALIGN_U32_CD( db_measurebase[0].db_measure.statistical.demand.realtime.Ic,             REG_ORDER_LITTLE_EDIAN), // 4
    REG_READ_ALIGN_U32_AB( db_measurebase[0].db_measure.statistical.demand.realtime.Ic,             REG_ORDER_LITTLE_EDIAN), // 5
    REG_READ_ALIGN_U32_CD( db_measurebase[0].db_measure.statistical.demand.realtime.Pa,             REG_ORDER_LITTLE_EDIAN), // 6
    REG_READ_ALIGN_U32_AB( db_measurebase[0].db_measure.statistical.demand.realtime.Pa,             REG_ORDER_LITTLE_EDIAN), // 7
    REG_READ_ALIGN_U32_CD( db_measurebase[0].db_measure.statistical.demand.realtime.Pb,             REG_ORDER_LITTLE_EDIAN), // 8
    REG_READ_ALIGN_U32_AB( db_measurebase[0].db_measure.statistical.demand.realtime.Pb,             REG_ORDER_LITTLE_EDIAN), // 9
    REG_READ_ALIGN_U32_CD( db_measurebase[0].db_measure.statistical.demand.realtime.Pc,             REG_ORDER_LITTLE_EDIAN), // 10
    REG_READ_ALIGN_U32_AB( db_measurebase[0].db_measure.statistical.demand.realtime.Pc,             REG_ORDER_LITTLE_EDIAN), // 11
    REG_READ_ALIGN_U32_CD( db_measurebase[0].db_measure.statistical.demand.realtime.Ptot,           REG_ORDER_LITTLE_EDIAN), // 12
    REG_READ_ALIGN_U32_AB( db_measurebase[0].db_measure.statistical.demand.realtime.Ptot,           REG_ORDER_LITTLE_EDIAN), // 13
    REG_READ_ALIGN_U32_CD( db_measurebase[0].db_measure.statistical.demand.realtime.Qa,             REG_ORDER_LITTLE_EDIAN), // 14
    REG_READ_ALIGN_U32_AB( db_measurebase[0].db_measure.statistical.demand.realtime.Qa,             REG_ORDER_LITTLE_EDIAN), // 15
    REG_READ_ALIGN_U32_CD( db_measurebase[0].db_measure.statistical.demand.realtime.Qb,             REG_ORDER_LITTLE_EDIAN), // 16
    REG_READ_ALIGN_U32_AB( db_measurebase[0].db_measure.statistical.demand.realtime.Qb,             REG_ORDER_LITTLE_EDIAN), // 17
    REG_READ_ALIGN_U32_CD( db_measurebase[0].db_measure.statistical.demand.realtime.Qc,             REG_ORDER_LITTLE_EDIAN), // 18
    REG_READ_ALIGN_U32_AB( db_measurebase[0].db_measure.statistical.demand.realtime.Qc,             REG_ORDER_LITTLE_EDIAN), // 19
    REG_READ_ALIGN_U32_CD( db_measurebase[0].db_measure.statistical.demand.realtime.Qtot,           REG_ORDER_LITTLE_EDIAN), // 20
    REG_READ_ALIGN_U32_AB( db_measurebase[0].db_measure.statistical.demand.realtime.Qtot,           REG_ORDER_LITTLE_EDIAN), // 21
    REG_READ_ALIGN_U32_CD( db_measurebase[0].db_measure.statistical.demand.realtime.Sa,             REG_ORDER_LITTLE_EDIAN), // 22
    REG_READ_ALIGN_U32_AB( db_measurebase[0].db_measure.statistical.demand.realtime.Sa,             REG_ORDER_LITTLE_EDIAN), // 23
    REG_READ_ALIGN_U32_CD( db_measurebase[0].db_measure.statistical.demand.realtime.Sb,             REG_ORDER_LITTLE_EDIAN), // 24
    REG_READ_ALIGN_U32_AB( db_measurebase[0].db_measure.statistical.demand.realtime.Sb,             REG_ORDER_LITTLE_EDIAN), // 25
    REG_READ_ALIGN_U32_CD( db_measurebase[0].db_measure.statistical.demand.realtime.Sc,             REG_ORDER_LITTLE_EDIAN), // 26
    REG_READ_ALIGN_U32_AB( db_measurebase[0].db_measure.statistical.demand.realtime.Sc,             REG_ORDER_LITTLE_EDIAN), // 27
    REG_READ_ALIGN_U32_CD( db_measurebase[0].db_measure.statistical.demand.realtime.Stot,           REG_ORDER_LITTLE_EDIAN), // 28
    REG_READ_ALIGN_U32_AB( db_measurebase[0].db_measure.statistical.demand.realtime.Stot,           REG_ORDER_LITTLE_EDIAN), // 29
    /* negative */
    REG_READ_ALIGN_U32_CD( db_measurebase[0].db_measure.statistical.demand.realtime.Pa_neg,         REG_ORDER_LITTLE_EDIAN), // 30
    REG_READ_ALIGN_U32_AB( db_measurebase[0].db_measure.statistical.demand.realtime.Pa_neg,         REG_ORDER_LITTLE_EDIAN), // 31
    REG_READ_ALIGN_U32_CD( db_measurebase[0].db_measure.statistical.demand.realtime.Pb_neg,         REG_ORDER_LITTLE_EDIAN), // 32
    REG_READ_ALIGN_U32_AB( db_measurebase[0].db_measure.statistical.demand.realtime.Pb_neg,         REG_ORDER_LITTLE_EDIAN), // 33
    REG_READ_ALIGN_U32_CD( db_measurebase[0].db_measure.statistical.demand.realtime.Pc_neg,         REG_ORDER_LITTLE_EDIAN), // 34
    REG_READ_ALIGN_U32_AB( db_measurebase[0].db_measure.statistical.demand.realtime.Pc_neg,         REG_ORDER_LITTLE_EDIAN), // 35
    REG_READ_ALIGN_U32_CD( db_measurebase[0].db_measure.statistical.demand.realtime.Ptot_neg,       REG_ORDER_LITTLE_EDIAN), // 36
    REG_READ_ALIGN_U32_AB( db_measurebase[0].db_measure.statistical.demand.realtime.Ptot_neg,       REG_ORDER_LITTLE_EDIAN), // 37
    REG_READ_ALIGN_U32_CD( db_measurebase[0].db_measure.statistical.demand.realtime.Qa_neg,         REG_ORDER_LITTLE_EDIAN), // 38
    REG_READ_ALIGN_U32_AB( db_measurebase[0].db_measure.statistical.demand.realtime.Qa_neg,         REG_ORDER_LITTLE_EDIAN), // 39
    REG_READ_ALIGN_U32_CD( db_measurebase[0].db_measure.statistical.demand.realtime.Qb_neg,         REG_ORDER_LITTLE_EDIAN), // 40
    REG_READ_ALIGN_U32_AB( db_measurebase[0].db_measure.statistical.demand.realtime.Qb_neg,         REG_ORDER_LITTLE_EDIAN), // 41
    REG_READ_ALIGN_U32_CD( db_measurebase[0].db_measure.statistical.demand.realtime.Qc_neg,         REG_ORDER_LITTLE_EDIAN), // 42
    REG_READ_ALIGN_U32_AB( db_measurebase[0].db_measure.statistical.demand.realtime.Qc_neg,         REG_ORDER_LITTLE_EDIAN), // 43
    REG_READ_ALIGN_U32_CD( db_measurebase[0].db_measure.statistical.demand.realtime.Qtot_neg,       REG_ORDER_LITTLE_EDIAN), // 44
    REG_READ_ALIGN_U32_AB( db_measurebase[0].db_measure.statistical.demand.realtime.Qtot_neg,       REG_ORDER_LITTLE_EDIAN), // 45
};







/* 用户配置重映射 */
struct remap_Modbus_node remapUserBaseCfg_meter1[] = {
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.user.base.Addr                               , REG_ORDER_LITTLE_EDIAN  ), // 0
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_cfg.user.base.BaudRate                           , REG_ORDER_LITTLE_EDIAN  ), // 1
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_cfg.user.base.BaudRate                           , REG_ORDER_LITTLE_EDIAN  ), // 2
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.user.base.CharSize                           , REG_ORDER_LITTLE_EDIAN  ), // 3
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.user.base.ParityBit                          , REG_ORDER_LITTLE_EDIAN  ), // 4
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.user.base.StopBit                            , REG_ORDER_LITTLE_EDIAN  ), // 5
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.user.base.WiringMode                         , REG_ORDER_LITTLE_EDIAN  ), // 6
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_cfg.user.base.PT                                 , REG_ORDER_LITTLE_EDIAN  ), // 7
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_cfg.user.base.PT                                 , REG_ORDER_LITTLE_EDIAN  ), // 8
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_cfg.user.base.CT                                 , REG_ORDER_LITTLE_EDIAN  ), // 9
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_cfg.user.base.CT                                 , REG_ORDER_LITTLE_EDIAN  ), // 10
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.user.base.PhaseAdjust1                       , REG_ORDER_LITTLE_EDIAN  ), // 11
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.user.base.PhaseAdjust2                       , REG_ORDER_LITTLE_EDIAN  ), // 12
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.user.base.PhaseAdjust3                       , REG_ORDER_LITTLE_EDIAN  ), // 13
    /* demand */
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.user.demand.mode                             , REG_ORDER_LITTLE_EDIAN  ), // 14
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.user.demand.window                           , REG_ORDER_LITTLE_EDIAN  ), // 15
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.user.demand.slip_window                      , REG_ORDER_LITTLE_EDIAN  ), // 16
    /* pulse */
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.user.pulseout.checked                        , REG_ORDER_LITTLE_EDIAN  ), // 17
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.user.pulseout.pulse[0].mode                  , REG_ORDER_LITTLE_EDIAN  ), // 18
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.user.pulseout.pulse[0].constant              , REG_ORDER_LITTLE_EDIAN  ), // 19
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.user.pulseout.pulse[0].wide                  , REG_ORDER_LITTLE_EDIAN  ), // 20
    /*reserved*/ 
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.user.base.Language                           , REG_ORDER_LITTLE_EDIAN  ), // 21
    REG_READ_ALIGN_U16   (db_null.dummy                                                         , REG_ORDER_LITTLE_EDIAN  ), // 22
    REG_READ_ALIGN_U16   (db_null.dummy                                                         , REG_ORDER_LITTLE_EDIAN  ), // 23
    REG_READ_ALIGN_U16   (db_null.dummy                                                         , REG_ORDER_LITTLE_EDIAN  ), // 24
    REG_READ_ALIGN_U16   (db_null.dummy                                                         , REG_ORDER_LITTLE_EDIAN  ), // 25
    REG_READ_ALIGN_U16   (db_null.dummy                                                         , REG_ORDER_LITTLE_EDIAN  ), // 26
    
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_cfg.user.refer.refer_voltage                     , REG_ORDER_LITTLE_EDIAN  ), // 27
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_cfg.user.refer.refer_voltage                     , REG_ORDER_LITTLE_EDIAN  ), // 28
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_cfg.user.refer.refer_current                     , REG_ORDER_LITTLE_EDIAN  ), // 29
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_cfg.user.refer.refer_current                     , REG_ORDER_LITTLE_EDIAN  ), // 30
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_cfg.user.refer.refer_frequency                   , REG_ORDER_LITTLE_EDIAN  ), // 31
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_cfg.user.refer.refer_frequency                   , REG_ORDER_LITTLE_EDIAN  ), // 32
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_cfg.user.refer.refer_Temperature                 , REG_ORDER_LITTLE_EDIAN  ), // 33
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_cfg.user.refer.refer_Temperature                 , REG_ORDER_LITTLE_EDIAN  ), // 34

    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.user.loadrun.obj                             , REG_ORDER_LITTLE_EDIAN  ), // 35
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_cfg.user.loadrun.over_I                          , REG_ORDER_LITTLE_EDIAN  ), // 36
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_cfg.user.loadrun.over_I                          , REG_ORDER_LITTLE_EDIAN  ), // 37
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_cfg.user.loadrun.over_V                          , REG_ORDER_LITTLE_EDIAN  ), // 38
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_cfg.user.loadrun.over_V                          , REG_ORDER_LITTLE_EDIAN  ), // 39
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_cfg.user.loadrun.under_V                         , REG_ORDER_LITTLE_EDIAN  ), // 40
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_cfg.user.loadrun.under_V                         , REG_ORDER_LITTLE_EDIAN  ), // 41
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_cfg.user.loadrun.over_Ptot                       , REG_ORDER_LITTLE_EDIAN  ), // 42
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_cfg.user.loadrun.over_Ptot                       , REG_ORDER_LITTLE_EDIAN  ), // 43

    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_cfg.user.network.ip_addr                         , REG_ORDER_LITTLE_EDIAN  ), // 44
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_cfg.user.network.ip_addr                         , REG_ORDER_LITTLE_EDIAN  ), // 45
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_cfg.user.network.subnet                          , REG_ORDER_LITTLE_EDIAN  ), // 46
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_cfg.user.network.subnet                          , REG_ORDER_LITTLE_EDIAN  ), // 47
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_cfg.user.network.gateway                         , REG_ORDER_LITTLE_EDIAN  ), // 48
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_cfg.user.network.gateway                         , REG_ORDER_LITTLE_EDIAN  ), // 49
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_cfg.user.network.dns                             , REG_ORDER_LITTLE_EDIAN  ), // 50
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_cfg.user.network.dns                             , REG_ORDER_LITTLE_EDIAN  ), // 51
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_cfg.user.network.dhcp                            , REG_ORDER_LITTLE_EDIAN  ), // 52
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_cfg.user.network.dhcp                            , REG_ORDER_LITTLE_EDIAN  ), // 53
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.user.network.mac_addr[0]                     , REG_ORDER_BIG_EDIAN     ), // 54
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.user.network.mac_addr[2]                     , REG_ORDER_BIG_EDIAN     ), // 55
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.user.network.mac_addr[4]                     , REG_ORDER_BIG_EDIAN     ), // 56
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.user.network.mac_addr[6]                     , REG_ORDER_BIG_EDIAN     ), // 57
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.user.network.mac_addr[8]                     , REG_ORDER_BIG_EDIAN     ), // 58
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.user.network.mac_addr[10]                    , REG_ORDER_BIG_EDIAN     ), // 59
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.user.network.mac_addr[12]                    , REG_ORDER_BIG_EDIAN     ), // 60
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.user.network.mac_addr[14]                    , REG_ORDER_BIG_EDIAN     ), // 61
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.user.network.mac_addr[16]                    , REG_ORDER_BIG_EDIAN     ), // 62
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_cfg.user.runing.setPtot                          , REG_ORDER_LITTLE_EDIAN  ), // 63
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_cfg.user.runing.setPtot                          , REG_ORDER_LITTLE_EDIAN  ), // 64
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_cfg.user.runing.setPa                            , REG_ORDER_LITTLE_EDIAN  ), // 65
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_cfg.user.runing.setPa                            , REG_ORDER_LITTLE_EDIAN  ), // 66
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_cfg.user.runing.setPb                            , REG_ORDER_LITTLE_EDIAN  ), // 67
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_cfg.user.runing.setPb                            , REG_ORDER_LITTLE_EDIAN  ), // 68
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_cfg.user.runing.setPc                            , REG_ORDER_LITTLE_EDIAN  ), // 69
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_cfg.user.runing.setPc                            , REG_ORDER_LITTLE_EDIAN  ), // 70
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_cfg.user.runing.setQtot                          , REG_ORDER_LITTLE_EDIAN  ), // 71
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_cfg.user.runing.setQtot                          , REG_ORDER_LITTLE_EDIAN  ), // 72
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_cfg.user.runing.setQa                            , REG_ORDER_LITTLE_EDIAN  ), // 73
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_cfg.user.runing.setQa                            , REG_ORDER_LITTLE_EDIAN  ), // 74
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_cfg.user.runing.setQb                            , REG_ORDER_LITTLE_EDIAN  ), // 75
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_cfg.user.runing.setQb                            , REG_ORDER_LITTLE_EDIAN  ), // 76
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_cfg.user.runing.setQc                            , REG_ORDER_LITTLE_EDIAN  ), // 77
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_cfg.user.runing.setQc                            , REG_ORDER_LITTLE_EDIAN  ), // 78
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_cfg.user.runing.setStot                          , REG_ORDER_LITTLE_EDIAN  ), // 79
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_cfg.user.runing.setStot                          , REG_ORDER_LITTLE_EDIAN  ), // 80
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_cfg.user.runing.setSa                            , REG_ORDER_LITTLE_EDIAN  ), // 81
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_cfg.user.runing.setSa                            , REG_ORDER_LITTLE_EDIAN  ), // 82
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_cfg.user.runing.setSb                            , REG_ORDER_LITTLE_EDIAN  ), // 83
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_cfg.user.runing.setSb                            , REG_ORDER_LITTLE_EDIAN  ), // 84
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_cfg.user.runing.setSc                            , REG_ORDER_LITTLE_EDIAN  ), // 85
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_cfg.user.runing.setSc                            , REG_ORDER_LITTLE_EDIAN  ), // 86
};                             


/* 
    系统校表配置重映射 (校正寄存器)
*/
struct remap_Modbus_node remapSysAdjustCfg_meter1[] = {
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.sys.adjust.HfConst1                          , REG_ORDER_LITTLE_EDIAN  ), // 0 
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.sys.adjust.GsUa                              , REG_ORDER_LITTLE_EDIAN  ), // 1 
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.sys.adjust.GsUb                              , REG_ORDER_LITTLE_EDIAN  ), // 2 
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.sys.adjust.GsUc                              , REG_ORDER_LITTLE_EDIAN  ), // 3 
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.sys.adjust.GsIa                              , REG_ORDER_LITTLE_EDIAN  ), // 4 
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.sys.adjust.GsIb                              , REG_ORDER_LITTLE_EDIAN  ), // 5 
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.sys.adjust.GsIc                              , REG_ORDER_LITTLE_EDIAN  ), // 6 
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_cfg.sys.adjust.Phs_Ua                            , REG_ORDER_LITTLE_EDIAN),   // 
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_cfg.sys.adjust.Phs_Ua                            , REG_ORDER_LITTLE_EDIAN),   //
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_cfg.sys.adjust.Phs_Ub                            , REG_ORDER_LITTLE_EDIAN),   //
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_cfg.sys.adjust.Phs_Ub                            , REG_ORDER_LITTLE_EDIAN),   //
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_cfg.sys.adjust.Phs_Uc                            , REG_ORDER_LITTLE_EDIAN),   //
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_cfg.sys.adjust.Phs_Uc                            , REG_ORDER_LITTLE_EDIAN),   //
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_cfg.sys.adjust.Phs_Ia                            , REG_ORDER_LITTLE_EDIAN),   //
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_cfg.sys.adjust.Phs_Ia                            , REG_ORDER_LITTLE_EDIAN),   //
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_cfg.sys.adjust.Phs_Ib                            , REG_ORDER_LITTLE_EDIAN),   //
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_cfg.sys.adjust.Phs_Ib                            , REG_ORDER_LITTLE_EDIAN),   //
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_cfg.sys.adjust.Phs_Ic                            , REG_ORDER_LITTLE_EDIAN),   //
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_cfg.sys.adjust.Phs_Ic                            , REG_ORDER_LITTLE_EDIAN),   //
    REG_READ_ALIGN_U32_CD(db_measurebase[0].db_cfg.sys.adjust.Phs_In                            , REG_ORDER_LITTLE_EDIAN),   //
    REG_READ_ALIGN_U32_AB(db_measurebase[0].db_cfg.sys.adjust.Phs_In                            , REG_ORDER_LITTLE_EDIAN),   //
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.sys.adjust.Pa_PhsL                           , REG_ORDER_LITTLE_EDIAN  ), // 7 
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.sys.adjust.Pb_PhsL                           , REG_ORDER_LITTLE_EDIAN  ), // 9 
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.sys.adjust.Pc_PhsL                           , REG_ORDER_LITTLE_EDIAN  ), // 11
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.sys.adjust.Qa_PhsL                           , REG_ORDER_LITTLE_EDIAN  ), // 13
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.sys.adjust.Qb_PhsL                           , REG_ORDER_LITTLE_EDIAN  ), // 15
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.sys.adjust.Qc_PhsL                           , REG_ORDER_LITTLE_EDIAN  ), // 17
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.sys.adjust.I0Adj                             , REG_ORDER_LITTLE_EDIAN  ), // 19
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.sys.adjust.T0Adj                             , REG_ORDER_LITTLE_EDIAN  ), // 20
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.sys.adjust.T1Adj                             , REG_ORDER_LITTLE_EDIAN  ), // 21
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.sys.adjust.T2Adj                             , REG_ORDER_LITTLE_EDIAN  ), // 22
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.sys.adjust.T3Adj                             , REG_ORDER_LITTLE_EDIAN  ), // 23

    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.sys.adjust.Offset_Ia                         , REG_ORDER_LITTLE_EDIAN  ), // 24
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.sys.adjust.Offset_Ib                         , REG_ORDER_LITTLE_EDIAN  ), // 25
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.sys.adjust.Offset_Ic                         , REG_ORDER_LITTLE_EDIAN  ), // 26
};


/* 系统参数，配置电参量的放大倍数 */
struct remap_Modbus_node  remapSysAdjustAdCfg_meter1[] = {
    REG_READ_ALIGN_U16(db_measurebase[0].db_cfg.sys.adjustAd.Adjust_Gv,     REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U16(db_measurebase[0].db_cfg.sys.adjustAd.Adjust_Gi,     REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U16(db_measurebase[0].db_cfg.sys.adjustAd.Krms_V   ,     REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U16(db_measurebase[0].db_cfg.sys.adjustAd.Krms_I   ,     REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U16(db_measurebase[0].db_cfg.sys.adjustAd.Krms_P   ,     REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U16(db_measurebase[0].db_cfg.sys.adjustAd.Krms_Pf  ,     REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U16(db_measurebase[0].db_cfg.sys.adjustAd.Krms_Hz  ,     REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U16(db_measurebase[0].db_cfg.sys.adjustAd.Krms_E   ,     REG_ORDER_LITTLE_EDIAN),
};
/*相位角补偿系数*/
struct remap_Modbus_node  remapSysPrthCfg_meter1[] = {
    REG_READ_ALIGN_U16(db_measurebase[0].db_cfg.sys.prthvalue.Prth1L  ,     REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U16(db_measurebase[0].db_cfg.sys.prthvalue.Prth1H  ,     REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U16(db_measurebase[0].db_cfg.sys.prthvalue.Prth2L  ,     REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U16(db_measurebase[0].db_cfg.sys.prthvalue.Prth2H  ,     REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U16(db_measurebase[0].db_cfg.sys.prthvalue.PhsxR1  ,     REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U16(db_measurebase[0].db_cfg.sys.prthvalue.PhsxR2  ,     REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U16(db_measurebase[0].db_cfg.sys.prthvalue.PhsxR3  ,     REG_ORDER_LITTLE_EDIAN),
};


/*校表的值，适配不同的硬件，不同的电流互感器*/
struct remap_Modbus_node remapSysCfgAdjustAdValue_meter1[] = {
    REG_READ_ALIGN_U16(    db_measurebase[0].db_cfg.sys.adjustAdvalue.Adjust_V ,        REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U16(    db_measurebase[0].db_cfg.sys.adjustAdvalue.Adjust_I ,        REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U16(    db_measurebase[0].db_cfg.sys.adjustAdvalue.Adjust_P ,        REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U32_CD( db_measurebase[0].db_cfg.sys.adjustAdvalue.Adjust_Va,        REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U32_AB( db_measurebase[0].db_cfg.sys.adjustAdvalue.Adjust_Va,        REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U32_CD( db_measurebase[0].db_cfg.sys.adjustAdvalue.Adjust_Vb,        REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U32_AB( db_measurebase[0].db_cfg.sys.adjustAdvalue.Adjust_Vb,        REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U32_CD( db_measurebase[0].db_cfg.sys.adjustAdvalue.Adjust_Vc,        REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U32_AB( db_measurebase[0].db_cfg.sys.adjustAdvalue.Adjust_Vc,        REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U32_CD( db_measurebase[0].db_cfg.sys.adjustAdvalue.Adjust_Ia,        REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U32_AB( db_measurebase[0].db_cfg.sys.adjustAdvalue.Adjust_Ia,        REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U32_CD( db_measurebase[0].db_cfg.sys.adjustAdvalue.Adjust_Ib,        REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U32_AB( db_measurebase[0].db_cfg.sys.adjustAdvalue.Adjust_Ib,        REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U32_CD( db_measurebase[0].db_cfg.sys.adjustAdvalue.Adjust_Ic,        REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U32_AB( db_measurebase[0].db_cfg.sys.adjustAdvalue.Adjust_Ic,        REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U32_CD( db_measurebase[0].db_cfg.sys.adjustAdvalue.Adjust_Pa,        REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U32_AB( db_measurebase[0].db_cfg.sys.adjustAdvalue.Adjust_Pa,        REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U32_CD( db_measurebase[0].db_cfg.sys.adjustAdvalue.Adjust_Pb,        REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U32_AB( db_measurebase[0].db_cfg.sys.adjustAdvalue.Adjust_Pb,        REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U32_CD( db_measurebase[0].db_cfg.sys.adjustAdvalue.Adjust_Pc,        REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U32_AB( db_measurebase[0].db_cfg.sys.adjustAdvalue.Adjust_Pc,        REG_ORDER_LITTLE_EDIAN),
};


/* 系统死区配置重映射 */
struct remap_Modbus_node remapSysDeadZoneCfg_meter1[] = {
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.sys.deadzone.Ua                              , REG_ORDER_LITTLE_EDIAN  ), // 0
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.sys.deadzone.Ub                              , REG_ORDER_LITTLE_EDIAN  ), // 1
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.sys.deadzone.Uc                              , REG_ORDER_LITTLE_EDIAN  ), // 2
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.sys.deadzone.Uab                             , REG_ORDER_LITTLE_EDIAN  ), // 3
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.sys.deadzone.Ubc                             , REG_ORDER_LITTLE_EDIAN  ), // 4
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.sys.deadzone.Uca                             , REG_ORDER_LITTLE_EDIAN  ), // 5
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.sys.deadzone.Ia                              , REG_ORDER_LITTLE_EDIAN  ), // 6
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.sys.deadzone.Ib                              , REG_ORDER_LITTLE_EDIAN  ), // 7
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.sys.deadzone.Ic                              , REG_ORDER_LITTLE_EDIAN  ), // 8
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.sys.deadzone.In                              , REG_ORDER_LITTLE_EDIAN  ), // 9
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.sys.deadzone.Pa                              , REG_ORDER_LITTLE_EDIAN  ), // 10
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.sys.deadzone.Pb                              , REG_ORDER_LITTLE_EDIAN  ), // 11
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.sys.deadzone.Pc                              , REG_ORDER_LITTLE_EDIAN  ), // 12
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.sys.deadzone.Qa                              , REG_ORDER_LITTLE_EDIAN  ), // 13
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.sys.deadzone.Qb                              , REG_ORDER_LITTLE_EDIAN  ), // 14
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.sys.deadzone.Qc                              , REG_ORDER_LITTLE_EDIAN  ), // 15
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.sys.deadzone.Sa                              , REG_ORDER_LITTLE_EDIAN  ), // 16
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.sys.deadzone.Sb                              , REG_ORDER_LITTLE_EDIAN  ), // 17
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.sys.deadzone.Sc                              , REG_ORDER_LITTLE_EDIAN  ), // 18
};

/* 系统死区配置重映射 */
struct remap_Modbus_node remapSysInterfaceCfg_meter1[] = {
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.sys.interface.RatedVoltage                   , REG_ORDER_LITTLE_EDIAN  ), // 0
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.sys.interface.RatedCurrent                   , REG_ORDER_LITTLE_EDIAN  ), // 1
    REG_READ_ALIGN_U16   (db_measurebase[0].db_cfg.sys.interface.RatedFrequency                 , REG_ORDER_LITTLE_EDIAN  ), // 2
};


struct remap_Modbus_node remapCommandAdjust_meter1[] = {
    REG_READ_ALIGN_U16(    db_measurebase[0].db_cfg.sys.adjustAd.Adjust_Gv,              REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U16(    db_measurebase[0].db_cfg.sys.adjustAd.Adjust_Gi,              REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U16(    db_measurebase[0].db_cfg.sys.adjustAdvalue.Adjust_V ,         REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U16(    db_measurebase[0].db_cfg.sys.adjustAdvalue.Adjust_I ,         REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U16(    db_measurebase[0].db_cfg.sys.adjustAdvalue.Adjust_P ,         REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U32_CD( db_measurebase[0].db_cfg.sys.adjustAdvalue.Adjust_Va,         REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U32_AB( db_measurebase[0].db_cfg.sys.adjustAdvalue.Adjust_Va,         REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U32_CD( db_measurebase[0].db_cfg.sys.adjustAdvalue.Adjust_Vb,         REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U32_AB( db_measurebase[0].db_cfg.sys.adjustAdvalue.Adjust_Vb,         REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U32_CD( db_measurebase[0].db_cfg.sys.adjustAdvalue.Adjust_Vc,         REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U32_AB( db_measurebase[0].db_cfg.sys.adjustAdvalue.Adjust_Vc,         REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U32_CD( db_measurebase[0].db_cfg.sys.adjustAdvalue.Adjust_Ia,         REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U32_AB( db_measurebase[0].db_cfg.sys.adjustAdvalue.Adjust_Ia,         REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U32_CD( db_measurebase[0].db_cfg.sys.adjustAdvalue.Adjust_Ib,         REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U32_AB( db_measurebase[0].db_cfg.sys.adjustAdvalue.Adjust_Ib,         REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U32_CD( db_measurebase[0].db_cfg.sys.adjustAdvalue.Adjust_Ic,         REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U32_AB( db_measurebase[0].db_cfg.sys.adjustAdvalue.Adjust_Ic,         REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U32_CD( db_measurebase[0].db_cfg.sys.adjustAdvalue.Adjust_Pa,         REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U32_AB( db_measurebase[0].db_cfg.sys.adjustAdvalue.Adjust_Pa,         REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U32_CD( db_measurebase[0].db_cfg.sys.adjustAdvalue.Adjust_Pb,         REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U32_AB( db_measurebase[0].db_cfg.sys.adjustAdvalue.Adjust_Pb,         REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U32_CD( db_measurebase[0].db_cfg.sys.adjustAdvalue.Adjust_Pc,         REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U32_AB( db_measurebase[0].db_cfg.sys.adjustAdvalue.Adjust_Pc,         REG_ORDER_LITTLE_EDIAN),
};



/*
    日需量最值
*/
struct remap_Modbus_block_node  remapMaxDmdDay_meter1[] = {
    REG_READ_ALIGN_BLOCK(db_measurebase[0].db_measure.statistical.demand.max.day, struct db_demand_max_base_type, REG_ORDER_LITTLE_EDIAN),
};

/*
    月需量最值
*/
struct remap_Modbus_block_node  remapMaxDmdMonth_meter1[] = {
    REG_READ_ALIGN_BLOCK(db_measurebase[0].db_measure.statistical.demand.max.month, struct db_demand_max_base_type, REG_ORDER_LITTLE_EDIAN),
};

/*
    最值
*/
struct remap_Modbus_block_node  remapExtremeDataDay_meter1[] = {
    REG_READ_ALIGN_BLOCK(db_measurebase[0].db_measure.statistical.extremum.day, struct db_extremum_basic_type, REG_ORDER_LITTLE_EDIAN),
};

struct remap_Modbus_block_node  remapExtremeDataMonth_meter1[] = {
    REG_READ_ALIGN_BLOCK(db_measurebase[0].db_measure.statistical.extremum.month, struct db_extremum_basic_type, REG_ORDER_LITTLE_EDIAN),
};

struct remap_Modbus_block_node  remapExtremeDataYear_meter1[] = {
    REG_READ_ALIGN_BLOCK(db_measurebase[0].db_measure.statistical.extremum.year, struct db_extremum_basic_type, REG_ORDER_LITTLE_EDIAN),
};

/*
    复费率电能
    块映射
*/
struct remap_Modbus_block_node remapEnergyMultirate_meter1[] = {
    REG_READ_ALIGN_BLOCK (db_measurebase[0].db_measure.energy.multirate                         , struct db_energy_multirate_type, REG_ORDER_LITTLE_EDIAN), // 0
};


/*
    谐波   
    块映射
*/
struct remap_Modbus_block_node remapPowerqualityHarmonicThd_meter1[] = {
    REG_READ_ALIGN_BLOCK (db_measurebase[0].db_measure.powerquality.harmonic.THD                , struct db_powerquality_harmonic_THD_type, REG_ORDER_LITTLE_EDIAN), // 0
};
struct remap_Modbus_block_node remapPowerqualityHarmonicHr_meter1[] = {
    REG_READ_ALIGN_BLOCK (db_measurebase[0].db_measure.powerquality.harmonic.HR                 , struct db_powerquality_harmonic_HR_type, REG_ORDER_LITTLE_EDIAN), // 0
};
struct remap_Modbus_block_node remapPowerqualityHarmonicRms_meter1[] = {
    REG_READ_ALIGN_BLOCK (db_measurebase[0].db_measure.powerquality.harmonic.RMS                , struct db_powerquality_harmonic_RMS_type, REG_ORDER_LITTLE_EDIAN), // 0
};
struct remap_Modbus_block_node remapPowerqualityHarmonicAngle_meter1[] = {
    REG_READ_ALIGN_BLOCK (db_measurebase[0].db_measure.powerquality.harmonic.Angle              , struct db_powerquality_harmonic_angle_type, REG_ORDER_LITTLE_EDIAN), // 0
};

