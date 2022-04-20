# TODO convert it to python implementation for more portable!

SUPPORTED_SOCS=${SOC_SUPPORTED_TARGETS}
GENERATED_LIBS="bk_cal bk_coex bk_hostapd bk_wifi_impl ble ble_5_x_rw ble_wifi_exchange bt ip \
		ip_ax mac_tx_cache net_param_intf power_save rf_test rwnx_intf uart_debug wpa_supplicant-2.9\
		arch ble_cli ble_pub controller hci host modules platform bk_airkiss sensor usb wfa_ca\
		bk_adapter pcm_resampler vad bk_system btdm_5_2_rw wolfssl mesh aec"

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

init_bk_libs_dir()
{
	rm -rf ${s_bk_libs_dir}/${s_soc}
	mkdir -p ${s_bk_libs_dir}/${s_soc}
	mkdir -p ${s_bk_libs_dir}/${s_soc}/libs
	mkdir -p ${s_bk_libs_dir}/${s_soc}/config
}

copy_libs()
{
	echo "Copy armino internal lib from ${s_armino_build_dir}/armino to ${s_armino_dir}/components/bk_libs"

	for component in ${GENERATED_LIBS}
	do
		if [ -f "${s_armino_build_dir}/armino/${component}/lib${component}.a" ]; then
			#echo "copy ${s_armino_build_dir}/armino/${component}/lib${component}.a"
			cp ${s_armino_build_dir}/armino/${component}/lib${component}.a ${s_bk_libs_dir}/${s_soc}/libs/
		fi
	done
	cd ..
}

copy_sdkconfig()
{
	cp ${s_armino_build_dir}/sdkconfig ${s_bk_libs_dir}/${s_soc}/config/
	cp ${s_armino_build_dir}/config/sdkconfig.h ${s_bk_libs_dir}/${s_soc}/config
}

exit_on_error()
{
	if [ $1 != 0 ]; then
	exit 1
	fi
}

s_soc=$1
s_armino_dir=$2
s_armino_build_dir=$3
s_bk_libs_dir="${s_armino_dir}/components/bk_libs"

init_bk_libs_dir
copy_libs
copy_sdkconfig 
