# TODO convert it to python implementation for more portable!

SUPPORTED_SOCS="bk7231n bk7231u bk7251 bk7271 bk7236 bk7256 bk7256_cp1"
GENERATED_LIBS="bk_cal bk_coex bk_hostapd bk_wifi_impl ble ble_5_x_rw ble_wifi_exchange bt ip \
		ip_ax mac_tx_cache net_param_intf power_save rf_test rwnx_intf uart_debug wpa_supplicant-2.9\
		arch ble_cli ble_pub controller hci host modules platform bk_airkiss sensor usb wfa_ca bk_rtos\
		bk_adapter pcm_resampler vad bk_system btdm_5_2_rw wolfssl mesh"

validate_soc()
{
        for soc in ${SUPPORTED_SOCS}
        do
            # echo $soc $1
            if [ $1 = $soc ]; then
                return 0
            fi
        done

        echo "Unknown soc $1!"
        return 1
}

copy_libs()
{
	soc=$1
	armino_dir=$2
	armino_build_dir=$3
	bk_libs_dir="${armino_dir}/components/bk_libs"
	echo "Copy armino internal lib from ${armino_build_dir}/armino to ${armino_dir}/components/bk_libs"

	mkdir -p ${bk_libs_dir}/${soc}

	for component in ${GENERATED_LIBS}
	do
		if [ -f "${armino_build_dir}/armino/${component}/lib${component}.a" ]; then
			#echo "copy ${armino_build_dir}/armino/${component}/lib${component}.a"
			cp ${armino_build_dir}/armino/${component}/lib${component}.a ${bk_libs_dir}/${soc}/lib${component}.a
		fi
	done
	cd ..
}

exit_on_error()
{
	if [ $1 != 0 ]; then
	exit 1
	fi
}

validate_soc $1
exit_on_error $?
copy_libs $1 $2 $3
