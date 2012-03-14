<?php
/**
 * @file
 * QIWI Respose Class
 */

class Response {
  public $updateBillResult;
}

class Param {
  public $login;
  public $password;
  public $txn;
  public $status;
}

class QiwiClientServer {
  function updateBill($param) {

    /* get store payment variables */
    $login = variable_get('uc_qiwi_mrh', array());
    $pass = variable_get('uc_qiwi_psw', array());
    $hash_string = md5($param->txn . drupal_strtoupper(md5($pass)));
    $hash = drupal_strtoupper($hash_string);

    /* check login and password */
    if ($login != $param->login || $hash != $param->password) {
      $temp = new Response();
      $temp->updateBillResult = 150;
      return $temp;
    }

    /* load current order */
    $order_id = $param->txn;
    $order = uc_order_load($order_id); // mb add is_numeric check?

    /* return error code 300 if order does't exist */
    if ($order === FALSE) {
      watchdog('qiwi', 'Invalid order number');
      $temp = new Response();
      $temp->updateBillResult = 300;
      return $temp;
    }

    /* Update order status to payment_received if code 60 (success) */
    if ($param->status == 60) {

      /* create payement desription */
      $description = t('A payment was accepted.');
      db_query("UPDATE {uc_qiwi_transaction} SET description = '%s' WHERE txnid = %d", $description, $order_id);

      /* update ubercart order */
      uc_payment_enter($order_id, 'qiwi', $amount, $order->uid, NULL, $description);
      uc_cart_complete_sale($order);
      uc_order_comment_save($order_id, 0, $description, 'admin');
      uc_order_update_status($order_id, 'payment_received'); //mb check correct end of func?

      watchdog('qiwi', 'Status changed to payment_received');

      /* return success code 0 if order success changed */
      $temp = new Response();
      $temp->updateBillResult = 0;
      return $temp;
    }

    elseif ($param->status > 100) {
      /* All status code more than 100 is cancelled payments */
      watchdog('qiwi_product', 'Change status: cancelled');

      /* return error code 300 */
      $temp = new Response();
      $temp->updateBillResult = 300;
      return $temp;
    }

    elseif ($param->status >= 50 && $param->status < 60) {
      /* Processing payment (usually, terminal payment) */
      $result = t('Order payment processing.');

      /* update ubercart order */
      uc_order_comment_save($order_id, 0, $result, 'admin');
      uc_order_update_status($order_id, 'processing');

      /* return not code 0, waiting new responses from Qiwi server */
      $temp = new Response();
      $temp->updateBillResult = 52;
      return $temp;
      }
      else {
      /* return error code 0, unknown status code */
      watchdog('qiwi_product', 'receive unknown status code - ' . check_plain($param->status));

      $temp = new Response();
      $temp->updateBillResult = 300;
      return $temp;
    }
  }
}