#pragma once
namespace vortex
{	
	auto io_dispatch( 
		PDEVICE_OBJECT device_object, 
		IRP* irp ) -> NTSTATUS
	{		
		UNREFERENCED_PARAMETER( device_object );	

		auto buffer = reinterpret_cast< invoke_data* >(
			irp->AssociatedIrp.SystemBuffer);
		
		switch ( buffer->code )
		{

		case invoke_base:
		{
			if ( request::get_module_base( buffer ) != driver::status::successful_operation )
			{
				irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
			}	
			break;
		}

		case invoke_read:
		{			
			if ( request::read_memory( buffer ) != driver::status::successful_operation )
			{
				irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
			}
			break;
		}

		case invoke_read_kernel: 
		{
			if ( request::mm_copy_kernel( buffer ) != driver::status::successful_operation )
			{
				irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
			}
			break;
		}

		case invoke_write:
		{
			if ( request::write_memory( buffer ) != driver::status::successful_operation )
			{
				irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
			}
			break;
		}

		case invoke_translate:
		{
			if ( request::translate_address( buffer ) != driver::status::successful_operation )
			{
				irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
			}
			break;
		}

		case invoke_dtb:
		{
			if ( request::get_dtb( buffer ) != driver::status::successful_operation )
			{
				irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
			}
			break;
		}
		case invoke_protect_virtual:
		{
			if (request::virtual_allocate( buffer ) != driver::status::successful_operation)
			{
				irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
			}
			break;
		}

		}

		irp->IoStatus.Status = STATUS_SUCCESS;
		qtx_import( IofCompleteRequest )(irp, IO_NO_INCREMENT);

		return STATUS_SUCCESS;
	}

	auto io_close( 
		PDEVICE_OBJECT device_object,
		IRP* irp ) -> long
	{
		UNREFERENCED_PARAMETER( device_object );

		irp->IoStatus.Status = STATUS_SUCCESS;
		irp->IoStatus.Information = 0;

		qtx_import(IofCompleteRequest)(irp, IO_NO_INCREMENT);
		return STATUS_SUCCESS;
	}
	
}
